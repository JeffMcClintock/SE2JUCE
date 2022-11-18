#include "./UnisonFix.h"
#include <climits>

REGISTER_PLUGIN ( UnisonFix, L"SE Unison Fix" );

#define NOTE_OFF                0x80
#define NOTE_ON                 0x90
#define SYSTEM_MSG				0xF0
#define POLY_AFTERTOUCH         0xA0
#define SYSTEM_EXCLUSIVE        0xF0
#define UNIVERSAL_REAL_TIME     0x7F
#define UNIVERSAL_NON_REAL_TIME 0x7E
#define SUB_ID_TUNING_STANDARD  0x08


UnisonFix::UnisonFix( IMpUnknown* host ) : MpBase( host )
,nextReleaseIndex(0)
{
	// Register pins.
	initializePin( 0, pinMIDIIn );
	initializePin( 1, pinMIDIOut );

	for( int midiKeyNumber = 0 ; midiKeyNumber < VirtualVoiceCount ; ++midiKeyNumber )
	{
		voiceReleaseOrder[midiKeyNumber] = 0;
		voiceChannel[midiKeyNumber] = 0;
		voiceKey[midiKeyNumber] = midiKeyNumber;

		for( int midiChannel = 0 ; midiChannel < 16 ; ++midiChannel )
		{
			// init as already mapped to correct key/pitch.
			keyMap[midiChannel][midiKeyNumber] = midiKeyNumber;
		}
	}
}

void UnisonFix::onMidiMessage( int pin, unsigned char* midiMessage, int size )
{
	int stat,b2,b3,midiChannel;// 3 bytes of MIDI message

	midiChannel = midiMessage[0] & 0x0f;

	stat = midiMessage[0] & 0xf0;
	b2 = midiMessage[1];
	b3 = midiMessage[2];

	// Note offs can be note_on vel=0
	if( b3 == 0 && stat == NOTE_ON )
	{
		stat = NOTE_OFF;
	}

	switch( stat )
	{
	case NOTE_ON:
		{
			int midiKeyNumber = midiMessage[1];
			int mappedMidiKeyNumber = keyMap[midiChannel][midiKeyNumber];

			// already mapped to a voice both ways?
			if( voiceKey[mappedMidiKeyNumber] == midiKeyNumber && voiceChannel[mappedMidiKeyNumber] == midiChannel)
			{
				// indicates in-use from two note-ons on same chan and key.
				if( voiceReleaseOrder[mappedMidiKeyNumber] == INT_MAX )
				{
					return; // prevent stuck notes.
				}
			}
			else	// find an unused voice.
			{
				int oldest = INT_MAX;

				// Find oldest released voice;
				for( int i = 0 ; i < VirtualVoiceCount ; ++i )
				{
					if( voiceReleaseOrder[i] < oldest )
					{
						oldest = voiceReleaseOrder[i];
						mappedMidiKeyNumber = i;
					}
				}

				// Map channel and Key Number to new virtual voice.
				keyMap[midiChannel][midiKeyNumber] = mappedMidiKeyNumber;
				voiceChannel[mappedMidiKeyNumber] = midiChannel;

				// need to re-tune?
				if( voiceKey[mappedMidiKeyNumber] != midiKeyNumber )
				{
					voiceKey[mappedMidiKeyNumber] = midiKeyNumber;

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

					// send tuning MIDI message for new key number (voice).
					unsigned char tuneMessage[] = { 0xF0, 0x7F, 0x00, 0x08, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF7 };
					tuneMessage[7] = mappedMidiKeyNumber; // MIDI key number (remapped key).
					int tune = GetIntKeyTune( midiKeyNumber );
					tuneMessage[8] = tune >> 14; // tuning course.
					tuneMessage[9] = (tune >> 7) & 0x7f; // tuning fine.
					tuneMessage[10] = tune & 0x7f; // tuning fine.

					pinMIDIOut.send( tuneMessage, sizeof(tuneMessage) );
				}
			}

			// send note-on MIDI message with new key number (voice).
			midiMessage[0] = NOTE_ON; // Force channel zero.
			midiMessage[1] = mappedMidiKeyNumber;
			pinMIDIOut.send( midiMessage, size, blockPosition() );

			voiceReleaseOrder[mappedMidiKeyNumber] = INT_MAX;
		}
		break;

	case NOTE_OFF:
		{
			int midiKeyNumber = midiMessage[1];
			int mappedMidiKeyNumber = keyMap[midiChannel][midiKeyNumber];

			// mapped to a voice both ways?
			if( voiceKey[mappedMidiKeyNumber] == midiKeyNumber && voiceChannel[mappedMidiKeyNumber] == midiChannel )
			{
				// send note-off MIDI message with assigned key number (voice).
				midiMessage[0] = NOTE_OFF; // Force channel zero.
				midiMessage[1] = mappedMidiKeyNumber;
				pinMIDIOut.send( midiMessage, size, blockPosition() );

				voiceReleaseOrder[mappedMidiKeyNumber] = nextReleaseIndex++;
			}
		}
		break;

	case POLY_AFTERTOUCH:
		{
			int midiKeyNumber = midiMessage[1];
			int mappedMidiKeyNumber = keyMap[midiChannel][midiKeyNumber];

			// mapped to a voice both ways?
			if( voiceKey[mappedMidiKeyNumber] == midiKeyNumber && voiceChannel[mappedMidiKeyNumber] == midiChannel )
			{
				// send note-off MIDI message with assigned key number (voice).
				midiMessage[0] = NOTE_OFF; // Force channel zero.
				midiMessage[1] = mappedMidiKeyNumber;
				pinMIDIOut.send( midiMessage, size, blockPosition() );
			}
		}
		break;

	case SYSTEM_MSG:
		{
			if( midiMessage[0] == SYSTEM_EXCLUSIVE &&
			  ( midiMessage[1] == UNIVERSAL_REAL_TIME || midiMessage[1] == UNIVERSAL_NON_REAL_TIME ) &&
			    midiMessage[3] == SUB_ID_TUNING_STANDARD )
			{
				OnMidiTuneMessage( blockPosition(), midiMessage );
			}
			else
			{
				pinMIDIOut.send( midiMessage, size, blockPosition() );
			}
		} 

	default:
		pinMIDIOut.send( midiMessage, size, blockPosition() );
		break;
	}
}

void UnisonFix::OnKeyTuningChanged(int block_relative_clock, int MidiKeyNumber, int tune )
{
	for( int midiChannel = 0 ; midiChannel < 16 ; ++ midiChannel )
	{
		int mappedMidiKeyNumber = keyMap[midiChannel][MidiKeyNumber];

		// mapped to a voice both ways?
		if( voiceKey[mappedMidiKeyNumber] == MidiKeyNumber && voiceChannel[mappedMidiKeyNumber] == midiChannel )
		{
			// send tuning MIDI message on mapped key.
			unsigned char tuneMessage[] = { 0xF0, 0x7F, 0x00, 0x08, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,0xF7};
			tuneMessage[8] = mappedMidiKeyNumber; // MIDI key number (remapped key).
			int tune = GetIntKeyTune( MidiKeyNumber );
			tuneMessage[9] = tune >> 14; // tuning course.
			tuneMessage[10] = (tune >> 7) & 0x7f; // tuning fine.
			tuneMessage[11] = tune & 0x7f; // tuning fine.

			pinMIDIOut.send( tuneMessage, sizeof(tuneMessage) );
			break;
		}
	}
}
