#include "./GuitarDechannelizer.h"

using namespace gmpi;

REGISTER_PLUGIN ( GuitarDechannelizer, L"SE Guitar De-Channelizer" );

GuitarDechannelizer::GuitarDechannelizer( IMpUnknown* host ) : MpBase( host ),
// init the midi converter
midiConverter(
	// provide a lambda to accept converted MIDI 2.0 messages
	[this](const midi::message_view& msg, int offset)
{
	onMidi2Message(msg);
}
)
{
	// Register pins.
	initializePin( 0, pinMIDIIn );
	initializePin( 1, pinMIDIOut );

	for( int chan = 0 ; chan < MidiChannelCount ; ++chan)
	{
		stringKeyNumber[chan] = chan;
		benderRange[chan] = 2.0f; // +/- 2 semitones default pitch bend.
		tuningOut[chan] = -1.0f; // -1 = not sent yet.

		// fill tuning table with default values.
		for (int midiKeyNumber = 0; midiKeyNumber < MidiKeyCount; ++midiKeyNumber)
		{
			tuningTable[chan][midiKeyNumber] = (float) midiKeyNumber;
		}
	}
}

// passes all MIDI to the converter.
void GuitarDechannelizer::onMidiMessage(int pin, unsigned char* midiMessage, int size)
{
	midi::message_view msg((const uint8_t*)midiMessage, size);

	// convert everything to MIDI 2.0
	midiConverter.processMidi(msg, -1);
}

void GuitarDechannelizer::onMidi2Message(const midi::message_view& msg)
{
	const auto header = gmpi::midi_2_0::decodeHeader(msg);

	// only 8-byte messages supported.
	if (header.messageType != gmpi::midi_2_0::ChannelVoice64)
		return;

	switch (header.status)
	{
	case gmpi::midi_2_0::PolyControlChange:
	{
		const auto polyController = gmpi::midi_2_0::decodePolyController(msg);

		if (polyController.type == gmpi::midi_2_0::PolyPitch)
		{
			const auto absolutePitchSemitones = gmpi::midi_2_0::decodeNotePitch(msg);
			tuningTable[header.channel][polyController.noteNumber] = absolutePitchSemitones;

			for(int string = 0 ; string < MidiChannelCount ; ++string)
			{
				if (stringKeyNumber[string] == polyController.noteNumber)
				{
					const auto out = gmpi::midi_2_0::makePolyController(
						string,
						gmpi::midi_2_0::PolyPitch,
						absolutePitchSemitones
					);
					pinMIDIOut.send(out.m);
				}
			}
		}
	}
	break;

	case gmpi::midi_2_0::NoteOn:
	{
		const auto note = gmpi::midi_2_0::decodeNote(msg);

		// MIDI chan 1 mapped to key-number 1, chan2 => key 2 etc.
		const int midiKeyNumber = note.noteNumber;
		const int guitarString = header.channel;

		if (gmpi::midi_2_0::attribute_type::Pitch == note.attributeType)
		{
			tuningTable[header.channel][midiKeyNumber] = note.attributeValue;
		}

//		// need to re-tune?
//		if (stringKeyNumber[guitarString] != midiKeyNumber)
//		{
//			stringKeyNumber[guitarString] = midiKeyNumber;
////			SendPitch(guitarString);
//		}
		stringKeyNumber[guitarString] = midiKeyNumber;

		// send note-on MIDI message with new key number (voice).
		//midiMessage[0] = NOTE_ON; // Force channel zero.
		//midiMessage[1] = guitarString;
		if (tuningOut[guitarString] != tuningTable[header.channel][midiKeyNumber])
		{
			tuningOut[guitarString] = tuningTable[header.channel][midiKeyNumber];

			const auto out = gmpi::midi_2_0::makeNoteOnMessageWithPitch(
				guitarString,
				note.velocity,
				tuningOut[guitarString]
			);

			pinMIDIOut.send(out.m);
		}
		else
		{
			const auto out = gmpi::midi_2_0::makeNoteOnMessage(
				guitarString,
				note.velocity
			);

			pinMIDIOut.send(out.m);
		}
	}
	break;

	case gmpi::midi_2_0::NoteOff:
	{
		const auto note = gmpi::midi_2_0::decodeNote(msg);
		const int guitarString = header.channel;

		// send note-off MIDI message with assigned key number (string number).
		//midiMessage[0] = NOTE_OFF; // Force channel zero.
		//midiMessage[1] = guitarString;

		const auto out = gmpi::midi_2_0::makeNoteOffMessage(
			guitarString,
			note.velocity
		);

		pinMIDIOut.send(out.m);
	}
	break;

	case gmpi::midi_2_0::PitchBend:
	{
		const int guitarString = header.channel;
		const auto normalized = gmpi::midi_2_0::decodeController(msg).value;
		bender[header.channel] = normalized * 2.0f - 1.f;

		const auto out = gmpi::midi_2_0::makePolyBender(
			guitarString,
			bender[header.channel]
		);

		//				_RPTN(0, "MPE: Bender %d: %f\n", keyInfo.MidiKeyNumber, channelBender[header.channel]);

		pinMIDIOut.send(out.m);
//		SendPitch(header.channel);
	}
	break;

	case gmpi::midi_2_0::RPN:
	{
		const auto rpn = gmpi::midi_2_0::decodeRpn(msg);
		const int guitarString = header.channel;

		if (rpn.rpn == gmpi::midi_2_0::RpnTypes::PitchBendSensitivity)
		{
			benderRange[guitarString] = rpn.value;
		}

		pinMIDIOut.send(msg.begin(), msg.size());
	}

	break;

	default:
		pinMIDIOut.send(msg.begin(), msg.size());
		break;
	};
}

#if 0
void GuitarDechannelizer::SendPitch(int guitarString)
{
	/*
		[SINGLE NOTE TUNING CHANGE (REAL-TIME)] 

		F0 7F <device ID> 08 02 tt ll [kk xx yy zz] F7 

		F0 7F  Universal Real Time SysEx header  
		<device ID>  ID of target device  
		08  sub-ID#1 (MIDI Tuning)  
		02  sub-ID#2 (note change)  
		tt  tuning program number (0 – 127)  
		ll  number of changes (1 change = 1 set of [kk xx yy zz])  
		[kk]  MIDI key number  
		[xx yy zz]  frequency data for that key (repeated ‘ll' number of times)  
		F7  EOX  
	*/
#if 0
	int tune = GetIntKeyTune( stringKeyNumber[guitarString] );
//	tune += (bender[keyNumber] / 0x2000 * benderRange[keyNumber] / 0x80 ) * 0x4000 ;
	tune += (bender[guitarString] * benderRange[guitarString] ) >> 6;

//	_RPT1(_CRT_WARN,"bender %f\n", (float)bender[guitarString]/(float)0x2000 );
//	_RPT1(_CRT_WARN,"bend amnt %f\n", (float)benderRange[guitarString] / (float)0x80 );


	// send tuning MIDI message for key number (voice).
	unsigned char tuneMessage[] = { 0xF0, 0x7F, 0x00, 0x08, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF7 };
	tuneMessage[7] = guitarString; // MIDI key number (remapped).
	tuneMessage[8] = tune >> 14; // tuning course.
	tuneMessage[9] = (tune >> 7) & 0x7f; // tuning fine.
	tuneMessage[10] = tune & 0x7f; // tuning fine.

	pinMIDIOut.send( tuneMessage, sizeof(tuneMessage) );
#else
	float tune = stringKeyNumber[guitarString];
	tune += bender[guitarString] * benderRange[guitarString];

	const auto out = gmpi::midi_2_0::makeNotePitchMessage(
		stringKeyNumber[guitarString],
		tune
	);
#endif
}

void GuitarDechannelizer::OnKeyTuningChanged( int p_clock, int MidiNoteNumber, int tune )
{
	for( int guitarString = 0 ; guitarString < MidiChannelCount ; ++guitarString )
	{
		// mapped to a currently playing voice?
		if( stringKeyNumber[guitarString] == MidiNoteNumber )
		{
			SendPitch(guitarString);
			break;
		}
	}
}

#endif