#include "math.h"
#include "./BpmTempo.h"
#include "./BpmTempo.xml.h"

SE_DECLARE_INIT_STATIC_FILE(BpmTempo) // old
SE_DECLARE_INIT_STATIC_FILE(BPMTempo)

REGISTER_PLUGIN ( BpmTempo, L"SE BPM Tempo" );

BpmTempo::BpmTempo( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( 0, pinHostBpm );
	initializePin( 1, pinHostTransport );
	initializePin( 2, pinBpm );
	initializePin( 3, pinTransport );
}

void BpmTempo::subProcess( int bufferOffset, int sampleFrames )
{
	// get pointers to in/output buffers.
	float* transport= bufferOffset + pinTransport.getBuffer();
	float* tempo	= bufferOffset + pinBpm.getBuffer();

	float lTransportRun;
	if( pinHostTransport )
	{
		lTransportRun = 1.0f;
	}
	else
	{
		lTransportRun = 0.0f;
	}

	float lTempo = pinHostBpm * 0.1f;
	for( int s = sampleFrames; s > 0; --s )
	{
		*tempo++ = lTempo;
		*transport++ = lTransportRun;
	}
}

void BpmTempo::onSetPins()
{
	if( pinHostTransport.isUpdated() )
	{
		pinTransport.setStreaming( false );
	}

	if( pinHostBpm.isUpdated() )
	{
		pinBpm.setStreaming( false );
	}

	// Set processing method.
	SET_PROCESS(&BpmTempo::subProcess);
}
