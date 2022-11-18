#include "./GuitarDechannelizer.h"

#define NOTE_OFF                0x80
#define NOTE_ON                 0x90
#define POLY_AFTERTOUCH         0xA0
#define CONTROL_CHANGE          0xB0
#define PITCHBEND               0xE0
#define SYSTEM_MSG				0xF0
#define SYSTEM_EXCLUSIVE        0xF0

#define UNIVERSAL_REAL_TIME     0x7F
#define UNIVERSAL_NON_REAL_TIME 0x7E
#define SUB_ID_TUNING_STANDARD  0x08

#define NRPN_MSB				99
#define NRPN_LSB				98
#define RPN_MSB					101
#define RPN_LSB					100
#define RPN_CONTROLLER			6
#define RPN_PITCH_BEND_SENSITIVITY  0x00

REGISTER_PLUGIN ( GuitarDechannelizer, L"SE Guitar De-Channelizer" );

void cntrl_update_msb(int& var, int hb)
{
	var = (var & 0x7f) + ( hb << 7 );	// mask off high bits and replace
}

void cntrl_update_lsb(int&var, int lb)
{
	var = ( var & 0x3F80) + lb;			// mask off low bits and replace
}


GuitarDechannelizer::GuitarDechannelizer( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( 0, pinMIDIIn );
	initializePin( 1, pinMIDIOut );

	for( int midiKeyNumber = 0 ; midiKeyNumber < MidiChannelCount ; ++midiKeyNumber )
	{
		stringKeyNumber[midiKeyNumber] = midiKeyNumber;
		benderRange[midiKeyNumber] = 2 * 0x80; // +/- 2 semitones default pitch bend.
		bender[midiKeyNumber] = 0;
	}
}

void GuitarDechannelizer::onMidiMessage( int pin, unsigned char* midiMessage, int size )
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

	// Pitch Bend.
	switch( stat )
	{
	case NOTE_ON:
		{
			// MIDI chan 1 mapped to key-number 1, chan2 => key 2 etc.
			int midiKeyNumber = midiMessage[1];
			int guitarString = midiChannel;

			// need to re-tune?
			if( stringKeyNumber[guitarString] != midiKeyNumber )
			{
				stringKeyNumber[guitarString] = midiKeyNumber;
				SendPitch(guitarString);
			}

			// send note-on MIDI message with new key number (voice).
			midiMessage[0] = NOTE_ON; // Force channel zero.
			midiMessage[1] = guitarString;
			pinMIDIOut.send( midiMessage, size, blockPosition() );
		}
		break;

	case NOTE_OFF:
		{
			int guitarString = midiChannel;

			// send note-off MIDI message with assigned key number (string number).
			midiMessage[0] = NOTE_OFF; // Force channel zero.
			midiMessage[1] = guitarString;
			pinMIDIOut.send( midiMessage, size, blockPosition() );
		}
		break;

	case PITCHBEND:
		{
			// _RPT0(_CRT_WARN,"pitch bend\n");
			int benderAmmount = (b3 << 7) + b2 - 0x2000;

			// store bend for this chan, for any note-ons that need it
			//float bend_amt = (float)bender_pos / (float) 0x2000;
			//assert( midiChannel >= 0 && midiChannel < 16 );

			bender[midiChannel] = benderAmmount;

			SendPitch(midiChannel);
		}
		break;
		
		/*
	case POLY_AFTERTOUCH:
		{
			int midiKeyNumber = midiMessage[1];
			int mappedMidiKeyNumber = keyMap[midiChannel][midiKeyNumber];

			// mapped to a voice both ways?
			if( stringKeyNumber[mappedMidiKeyNumber] == midiKeyNumber && voiceChannel[mappedMidiKeyNumber] == midiChannel )
			{
				// send note-off MIDI message with assigned key number (voice).
				midiMessage[0] = NOTE_OFF; // Force channel zero.
				midiMessage[1] = mappedMidiKeyNumber;
				pinMIDIOut.send( midiMessage, size, blockPosition() );
			}
		}
		break;
*/
	case CONTROL_CHANGE:
		//_RPT0(_CRT_WARN,"control change\n");
		switch( b2 )
		{
		case NRPN_MSB:
		case NRPN_LSB:
			incoming_rpn = -1;	// ignore nrpn msgs
			break;

		case RPN_MSB:
			cntrl_update_msb( incoming_rpn, b3 );
			break;

		case RPN_LSB:
			cntrl_update_lsb( incoming_rpn, b3 );
			break;

		case RPN_CONTROLLER:	// data entry slider MSB
			if( incoming_rpn == RPN_PITCH_BEND_SENSITIVITY )
			{
				// has to be stored per channel!!!
				cntrl_update_msb( benderRange[midiChannel], b3 );
//				chan_info[midiChannel].midi_bend_range = (float) pitch_bend_sensitity_raw / (float) 0x80;
			}
			break;
		case  38:	// data entry slider LSB
			if( incoming_rpn == RPN_PITCH_BEND_SENSITIVITY )
			{
				cntrl_update_lsb( benderRange[midiChannel], b3 );
//				benderRange[midiChannel] = (float) pitch_bend_sensitity_raw / (float) 0x80;
			}
			break;
		case 64: // Hold Pedal
		case 69:
			{
			}
			break;
		case 120:	// All sound off
		case 123:	// All notes off
			{
			}
			break;
		};

		pinMIDIOut.send( midiMessage, size );

		break;

	case SYSTEM_MSG:
		{
			if( midiMessage[0] == SYSTEM_EXCLUSIVE &&
			  ( midiMessage[1] == UNIVERSAL_REAL_TIME || midiMessage[1] == UNIVERSAL_NON_REAL_TIME ) &&
			    midiMessage[3] == SUB_ID_TUNING_STANDARD )
			{
				OnMidiTuneMessage( -1, midiMessage );
			}
			else
			{
				pinMIDIOut.send( midiMessage, size );
			}
		}
		break;

	default:
		pinMIDIOut.send( midiMessage, size );
		break;
	}
}

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

