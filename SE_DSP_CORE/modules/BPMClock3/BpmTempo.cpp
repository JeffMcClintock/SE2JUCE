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
	initializePin(pinProcessorResumedIn);
	initializePin(pinProcessorResumed);
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

	if (pinProcessorResumed.getValue())
	{
		assert(sampleFrames > 0);
		pinProcessorResumed.setValue(false, bufferOffset + 1); // single sample pulse
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

	// In FL Studio clip effects especially, the DAW can pause the plugin at the end of the clip, and not resume calling it until the next time it plays the start of the clip.
	// this results in 'tails' playing at the next *start* of the clip. To avoid this, we clear the tails when the plugin is paused.
	if (pinProcessorResumedIn.isUpdated())
	{
		pinProcessorResumed = true;

		// since bool pins don't cause subProcess to be called, we need to wake up subprocess at least one time,
		// to reset this bool pin back to 'false'.
		wakeSubProcessAtLeastOnce();
	}

	// Set processing method.
	SET_PROCESS(&BpmTempo::subProcess);
}
