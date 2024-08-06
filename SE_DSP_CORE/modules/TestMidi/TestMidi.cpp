#include ".\TestMidi.h"

REGISTER_PLUGIN ( TestMidi, L"SE TestMidi" );

TestMidi::TestMidi( IMpUnknown* host ) : MpBase( host )
,count(0)
, key(0)
{
	// Register pins.
	initializePin( 0, pinMidiOut );

	SET_PROCESS( &TestMidi::subProcess );
	setSleep( false );
}

void TestMidi::subProcess( int bufferOffset, int sampleFrames )
{
	const int dura = 388;

	for( int s = sampleFrames; s > 0; --s )
	{
		if( count-- < 0 )
		{
			for( int n = 0 ; n < 3 ; ++n )
			{
				count = dura;
				unsigned char midiMsg[4];
				midiMsg[0] = 0x90; // NOTE_ON
				midiMsg[1] = key & 0x7f; // key-num
				midiMsg[2] = 0x90; // velocity

				pinMidiOut.send( midiMsg, 3, bufferOffset + sampleFrames - s );

				midiMsg[0] = 0x80; // NOTE_OFF
				midiMsg[1] = (key-1) & 0x7f; // key-num

				pinMidiOut.send( midiMsg, 3, bufferOffset + sampleFrames - s );

				key++;

				if( key > 9 )
				{
					SET_PROCESS( &TestMidi::subProcessNothing );
					return;
				}
			}
		}
	}
}
