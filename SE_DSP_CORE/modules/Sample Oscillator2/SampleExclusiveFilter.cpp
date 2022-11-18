#include "./SampleExclusiveFilter.h"

REGISTER_PLUGIN ( SampleExclusiveFilter, L"SE Sample Exclusive Hihat" );

SampleExclusiveFilter::SampleExclusiveFilter( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( 0, pinMidiIn );
	initializePin( 1, pinMidiOut );
}

void SampleExclusiveFilter::onMidiMessage( int pin, unsigned char* midiMessage, int size )
{
	// size = 4 for short msg (4 or less), or > 4 for sysex
	//_RPTW1(_CRT_WARN, L"onMidiMessage pin %d\n[", pin);
	//for(int i = 0 ; i < size ; i++)
	//{
	//	_RPT1(_CRT_WARN, " %2x", (int) midiMessage[i]);
	//}
	//_RPT0(_CRT_WARN, "]\n");

	const unsigned char openHihat = 46;
	const unsigned char pedalHihat = 44;
	const unsigned char closedHihat = 42;
	const unsigned char SYSTEM_MSG = 0xF0;
	const unsigned char NOTE_ON = 0x90;
	const unsigned char allSoundOff = 0x78;

	int stat,b1,b2,b3,chan;

	stat = midiMessage[0] & 0xf0;
	bool is_system_msg = ( stat & SYSTEM_MSG ) == SYSTEM_MSG;

	if( is_system_msg )
	{
		goto pass_filter;
	}

	chan = midiMessage[0] & 0x0f;

	b1 = midiMessage[0];
	b2 = midiMessage[1];
	b3 = midiMessage[2];

	if( stat == NOTE_ON && b3 != 0 )  // b3 != 0 tests for note-offs (note on vel = 0)
	{
		if( b2 == openHihat || b2 == pedalHihat || b2 == closedHihat )
		{
			for( int keyNumber = closedHihat ; keyNumber <= openHihat ; keyNumber += 2 )
			{
				if( keyNumber != b2 )
				{
					/*
						F0 7F Universal Real Time SysEx header
						<device ID> ID of target device (7F = all devices)
						0A sub-ID#1 = “Key-Based Instrument Control”
						01 sub-ID#2 = 01 Basic Message
						0n MIDI Channel Number
						kk Key number
						[nn,vv] Controller Number and Value
						:
						F7 EOX
					*/
					unsigned char midiMessage2[] = { 0xF0, 0x7f, 0x7f, 0x0A, 0x01, 0x00, (unsigned char)keyNumber, allSoundOff, 0x7f };
					pinMidiOut.send( midiMessage2, sizeof(midiMessage2) );
				}
			}
		}
	}

pass_filter:

	// echo MIDI
	pinMidiOut.send( midiMessage, size );
}
