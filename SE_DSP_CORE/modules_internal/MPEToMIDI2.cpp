#include "mp_sdk_audio.h"
#include "mp_midi.h"

using namespace gmpi;

SE_DECLARE_INIT_STATIC_FILE(MPEToMIDI2)

namespace gmpi
{
namespace midi
{
}

namespace midi_1_0
{

}

namespace midi_2_0
{



} // midi 2
} //gmpi

// TODO !!! include/test on macOS.




class MPEToMIDI2 final : public MpBase2, public gmpi::midi_2_0::NoteMapper
{
	MidiInPin pinMIDIIn;
	MidiOutPin pinMIDIOut;
	IntInPin pinMode;

	// normalized bender for each chan. center = 0.5
	float channelBender[16];
	float channelBrightness[16];
	float channelPressure[16];

	int lowerZoneMasterChannel = 0;
	int upperZoneMasterChannel = -1;

	enum {Mode_passthrough, Mode_MPE};
public:
	MPEToMIDI2()
	{
		initializePin( pinMIDIIn );
		initializePin( pinMIDIOut );
		initializePin( pinMode );

		std::fill(std::begin(channelBender),     std::end(channelBender),     0.5f);
		std::fill(std::begin(channelBrightness), std::end(channelBrightness), 0.0f);
		std::fill(std::begin(channelPressure),   std::end(channelPressure),   0.0f);
	}

	void onMidiMessage(int /*pin*/, const unsigned char* midiMessage, int size) override
    {
		midi::message_view msg((const uint8_t*) midiMessage, size);

		if (Mode_passthrough == pinMode.getValue() || gmpi::midi_2_0::isMidi2Message(msg))
		{
			pinMIDIOut.send(midiMessage, size);
			return;
		}

		const auto header = midi_1_0::decodeHeader(msg);

		// 'Master' MIDI Channels are reserved for conveying messages that apply to the entire Zone.
		if (header.channel == lowerZoneMasterChannel || header.channel == upperZoneMasterChannel)
		{
			pinMIDIOut.send(midiMessage, size);
			return;
		}

		// for an MPE member channel, only note on/off, pitch-bend, and brightness are allowed.
		switch (header.status)
		{
		case midi_1_0::status_type::NoteOn:
		{
			auto note = midi_1_0::decodeNote(msg);
			const auto MpeId = note.noteNumber | (header.channel << 7);
			auto& keyInfo = allocateNote(MpeId, note.noteNumber);

//			_RPTN(0, "MPE Note-on %d => %d\n", note.noteNumber, keyInfo.MidiKeyNumber);

			// MIDI 2.0 does not automatically reset per-note controls. MPE is expected to.

			// reset per-note bender
			{
				const auto out = gmpi::midi_2_0::makePolyBender(
					keyInfo.MidiKeyNumber,
					channelBender[header.channel]
				);

//				_RPTN(0, "MPE: Bender %d: %f\n", keyInfo.MidiKeyNumber, channelBender[header.channel]);

				pinMIDIOut.send((const unsigned char*)&out, sizeof(out));				
			}

			// reset per-note brightness
			{
				const auto out = gmpi::midi_2_0::makePolyController(
					keyInfo.MidiKeyNumber,
					gmpi::midi_2_0::PolySoundController5, // Brightness
					channelBrightness[header.channel]
				);

//				_RPTN(0, "MPE: Brightness %d: %f\n", keyInfo.MidiKeyNumber, channelBrightness[header.channel]);

				pinMIDIOut.send((const unsigned char*)&out, sizeof(out));				
			}

			// reset per-note pressure
			{
				const auto out = gmpi::midi_2_0::makePolyPressure(
					keyInfo.MidiKeyNumber,
					channelPressure[header.channel]
				);

//				_RPTN(0, "MPE: Pressure %d: %f\n", keyInfo.MidiKeyNumber, channelPressure[header.channel]);

				pinMIDIOut.send((const unsigned char*)&out, sizeof(out));				
			}

			if (keyInfo.pitch != note.noteNumber) // !!! TODO: pitch from tuning_table[note_num], not just note_num. (and float)
			{
				keyInfo.pitch = note.noteNumber;
//						_RPTN(0, "      ..pitch = %d\n", (int)keyInfo.pitch);

				// !!! problem!!! w SynthEdit retaining tuning
				// This override is valid only for the one Note containing the Attribute #3: Pitch 7.9; it is not valid for any subsequent Notes
				const auto out = gmpi::midi_2_0::makeNoteOnMessageWithPitch(
					keyInfo.MidiKeyNumber,
					note.velocity,
					keyInfo.pitch
				);

				pinMIDIOut.send((const unsigned char*)&out, sizeof(out));
			}
			else
			{
//				_RPTN(0, "      ..existing pitch = %d\n", (int)keyInfo.pitch);
				const auto out = gmpi::midi_2_0::makeNoteOnMessage(
					keyInfo.MidiKeyNumber,
					note.velocity
				);

				pinMIDIOut.send((const unsigned char*)&out, sizeof(out));
			}
		}
		break;

		case midi_1_0::status_type::NoteOff:
		{
			auto note = midi_1_0::decodeNote(msg);
			const auto MpeId = note.noteNumber | (header.channel << 7);
			auto keyInfo = findNote(MpeId);

			if (keyInfo)
			{
				keyInfo->held = false;

				const auto out = gmpi::midi_2_0::makeNoteOffMessage(
					keyInfo->MidiKeyNumber,
					note.velocity
				);

				pinMIDIOut.send((const unsigned char*)&out, sizeof(out));
			}
		}
		break;

		case midi_1_0::status_type::PitchBend:
		{
			channelBender[header.channel] = midi_1_0::decodeBender(msg);

			// find whatever note is playing on this channel. Assumption is only held notes can receive benders.
			// might not hold true for DAW automation which is drawn-on after note-off time
			for (auto& info : noteIds)
			{
				// "The note will cease to be affected by Pitch Bend messages on its Channel after the Note Off message occurs" - spec.
				if (info.held && header.channel == (info.noteId >> 7))
				{
					const auto out = gmpi::midi_2_0::makePolyBender(
						info.MidiKeyNumber,
						channelBender[header.channel]
					);

//					_RPTN(0, "MPE: Bender %d: %f\n", info.MidiKeyNumber, channelBender[header.channel]);

					pinMIDIOut.send((const unsigned char*)&out, sizeof(out));
				}
			}
		}
		break;

		case midi_1_0::status_type::ChannelPressure:
		{
			channelPressure[header.channel] = midi::utils::U7ToFloat(msg[1]);

			// find whatever note is playing on this channel. Assumption is only held notes can receive benders etc.
			// might not hold true for DAW automation which is drawn-on after note-off time
			for (auto& info : noteIds)
			{
				if (info.held && header.channel == (info.noteId >> 7))
				{
//					_RPTN(0, "MPE: Pressure %d %f\n", info.MidiKeyNumber, normalised);
					const auto out = gmpi::midi_2_0::makePolyPressure(
						info.MidiKeyNumber,
						channelPressure[header.channel]
					);
					pinMIDIOut.send((const unsigned char*)&out, sizeof(out));
				}
			}
		}
		break;

		case midi_1_0::status_type::ControlChange:
		{
			// 74 brightness
			if (74 != msg[1])
				break;

			channelBrightness[header.channel] = midi::utils::U7ToFloat(msg[2]);

			// find whatever note is playing on this channel. Assumption is only held notes can receive benders etc.
			// might not hold true for DAW automation which is drawn-on after note-off time
			for (auto& info : noteIds)
			{
				if (info.held && header.channel == (info.noteId >> 7))
				{
					const auto out = gmpi::midi_2_0::makePolyController(
						info.MidiKeyNumber,
						gmpi::midi_2_0::PolySoundController5, // Brightness
						channelBrightness[header.channel]
					);
					pinMIDIOut.send((const unsigned char*)&out, sizeof(out));

//					_RPTN(0, "MPE: Brightness %d %f (%d)\n", info.MidiKeyNumber, channelBrightness[header.channel], msg[2]);
				}
			}
		}
		break;
		}
    }

	void onSetPins() override
	{
		// Check which pins are updated.
		if( pinMode.isUpdated())
		{
			/*
			When a receiver receives an MPE Configuration Message, it must set the Master Pitch Bend Sensitivity to
			±2 semitones, and the Pitch Bend Sensitivity of the Member Channels to ±48 semitones.
			*/

			// set bender range for MPE.
			for (int chan = 0; chan < 16; ++chan)
			{
				int bendRangeSemitones = 48;
				if (Mode_passthrough == pinMode.getValue() || chan == lowerZoneMasterChannel || chan == upperZoneMasterChannel)
				{
					bendRangeSemitones = 2;
				}

				const auto out = gmpi::midi_2_0::makeRpnRaw(
					gmpi::midi_2_0::RpnTypes::PitchBendSensitivity,
					bendRangeSemitones << 18 // top 8 bits of 14 bit value into top byte. next 6 bits in 2nd msb.
				);

				pinMIDIOut.send((const unsigned char*)&out, sizeof(out));
			}

			// might want to send all-notes-off
		}
	}
};

namespace
{
	auto r = Register<MPEToMIDI2>::withXml(R"XML(
<?xml version="1.0" ?>
<PluginList>
  <Plugin id="SE MPE to MIDI2" name="MPE to MIDI2" category="MIDI">
    <Audio>
      <Pin name="MIDI In" datatype="midi" />
      <Pin name="MIDI Out" datatype="midi" direction="out" />
      <Pin name="MPE Mode" datatype="enum" metadata="MPE Off, MPE On" />
    </Audio>
  </Plugin>
</PluginList>
)XML");
}
