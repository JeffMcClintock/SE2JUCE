#include "../se_sdk3/mp_sdk_audio.h"
#include "../se_sdk3/mp_midi.h"
#include <array>

SE_DECLARE_INIT_STATIC_FILE(MIDItoGate)

using namespace gmpi;

class MidiToGate : public MpBase2
{
	MidiInPin pinMIDIIn;
	BoolOutPin pinGate;
	BoolInPin pinRetrigger;

	std::array<bool, 128> keyStates;
	int retriggerCounter_ = 0;
	gmpi::midi_2_0::MidiConverter2 midiConverter;

public:
	MidiToGate() :
		// init the midi converter
		midiConverter(
			// provide a lambda to accept converted MIDI 2.0 messages
			[this](const midi::message_view& msg, int offset)
			{
				onMidi2Message(msg);
			}
		)
	{
		initializePin( pinMIDIIn );
		initializePin( pinRetrigger );
		initializePin( pinGate );

		keyStates.fill(false);
	}

	void subProcess( int sampleFrames )
	{
		if (sampleFrames > retriggerCounter_)
		{
			pinGate.setValue(true, getBlockPosition() + retriggerCounter_);
			setSleep(true);
			setSubProcess(&MidiToGate::subProcessNothing);
		}

		retriggerCounter_ -= sampleFrames;
	}
#if 0 // old MIDI 1.0
	void onMidiMessage(int pin, unsigned char* midiMessage, int size)
	{
		int stat, b2, b3, midiChannel; // 3 bytes of MIDI message

		midiChannel = midiMessage[0] & 0x0f;

		stat = midiMessage[0] & 0xf0;
		b2 = midiMessage[1];
		b3 = midiMessage[2];

		// Note offs can be note_on vel=0
		if (b3 == 0 && stat == GmpiMidi::MIDI_NoteOn)
		{
			stat = GmpiMidi::MIDI_NoteOff;
		}

		switch (stat)
		{
		case GmpiMidi::MIDI_NoteOn:
		{
			keyStates[b2] = true;

			if (pinGate && pinRetrigger)
			{
				pinGate = false;
				retriggerCounter_ = 3;
				setSleep(false);
				setSubProcess(&MidiToGate::subProcess);
			}
			else
			{
				pinGate = true;
			}
		}
		break;

		case GmpiMidi::MIDI_NoteOff:
		{
			bool g = false;
			keyStates[b2] = false;
			for (auto keyState : keyStates)
			{
				if (keyState)
				{
					g = true;
					break;
				}
			}
			if (!g)
			{
				pinGate = false;
			}
		}
		break;

		default:
			break;
		}
	}
#endif

	// passes all MIDI to the converter.
	void onMidiMessage(int pin, unsigned char* midiMessage, int size) override
	{
		midi::message_view msg((const uint8_t*)midiMessage, size);

		// convert everything to MIDI 2.0
		midiConverter.processMidi(msg, -1);
	}

	// put your midi handling code in here.
	void onMidi2Message(const midi::message_view& msg)
	{
		const auto header = gmpi::midi_2_0::decodeHeader(msg);

		// only 8-byte messages supported.
		if (header.messageType != gmpi::midi_2_0::ChannelVoice64)
			return;

		switch (header.status)
		{

		case gmpi::midi_2_0::NoteOn:
		{
			const auto note = gmpi::midi_2_0::decodeNote(msg);

			keyStates[note.noteNumber] = true;

			if (pinGate && pinRetrigger)
			{
				pinGate = false;
				retriggerCounter_ = 3;
				setSleep(false);
				setSubProcess(&MidiToGate::subProcess);
			}
			else
			{
				pinGate = true;
			}
		}
		break;

		case gmpi::midi_2_0::NoteOff:
		{
			const auto note = gmpi::midi_2_0::decodeNote(msg);

			bool g = false;
			keyStates[note.noteNumber] = false;
			for (auto keyState : keyStates)
			{
				if (keyState)
				{
					g = true;
					break;
				}
			}
			if (!g)
			{
				pinGate = false;
			}
		}
		break;

		default:
			break;
		};
	}
};

namespace
{
	auto r = Register<MidiToGate>::withId(L"SE MIDItoGate");
}

class MidiToGate2 : public MpBase2
{
	MidiInPin pinMIDIIn;
	BoolOutPin pinTrigger;
	BoolOutPin pinGate;

	std::array<bool, 256> keyStates;
	int triggerCounter = 0;
	int triggerDuration = 0;

	gmpi::midi_2_0::MidiConverter2 midiConverter;

public:
	MidiToGate2() :
		// init the midi converter
		midiConverter(
			// provide a lambda to accept converted MIDI 2.0 messages
			[this](const midi::message_view& msg, int offset)
			{
				onMidi2Message(msg);
			}
		)
	{
		initializePin(pinMIDIIn);
		initializePin(pinTrigger);
		initializePin(pinGate);

		keyStates.fill(false);
	}

	int32_t open() override
	{
		MpBase2::open();	// always call the base class

		triggerDuration = static_cast<int>(getSampleRate() * 0.0005f); // 0.5 ms trigger pulse.

		return gmpi::MP_OK;
	}

	void subProcess(int sampleFrames)
	{
		for (int s = 0 ; s < sampleFrames; ++s)
		{
			if (triggerCounter-- == 0)
			{
				pinTrigger.setValue(false, getBlockPosition() + s);
				setSleep(true);
				setSubProcess(&MidiToGate::subProcessNothing);
				return;
			}
		}
	}

	// passes all MIDI to the converter.
	void onMidiMessage(int pin, unsigned char* midiMessage, int size) override
	{
		midi::message_view msg((const uint8_t*)midiMessage, size);

		// convert everything to MIDI 2.0
		midiConverter.processMidi(msg, -1);
	}

	// put your midi handling code in here.
	void onMidi2Message(const midi::message_view& msg)
	{
		const auto header = gmpi::midi_2_0::decodeHeader(msg);

		// only 8-byte messages supported. only 16 channels supported
		if (header.messageType != gmpi::midi_2_0::ChannelVoice64)
			return;

		switch (header.status)
		{

		case gmpi::midi_2_0::NoteOn:
		{
			const auto note = gmpi::midi_2_0::decodeNote(msg);

			keyStates[note.noteNumber] = true;

			pinGate = true;
			pinTrigger = true;
			triggerCounter = triggerDuration;

			setSleep(false);
			setSubProcess(&MidiToGate2::subProcess);
		}
		break;

		case gmpi::midi_2_0::NoteOff:
		{
			const auto note = gmpi::midi_2_0::decodeNote(msg);

			bool g = false;
			keyStates[note.noteNumber] = false;
			for (auto keyState : keyStates)
			{
				if (keyState)
				{
					g = true;
					break;
				}
			}
			if (!g)
			{
				pinGate = false;
				pinTrigger = false;
			}
		}
		break;

		default:
			break;
		};
	}
};

namespace
{
	auto r2 = Register<MidiToGate2>::withId(L"SE MIDItoGate2");
}