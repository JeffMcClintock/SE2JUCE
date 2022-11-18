#include "./EnvRetriggerModeAutoSet.h"

REGISTER_PLUGIN ( EnvRetriggerModeAutoSet, L"EnvRetriggerModeAutoSet" );

enum { RTM_RESTART, RTM_PICKUP };

EnvRetriggerModeAutoSet::EnvRetriggerModeAutoSet( IMpUnknown* host ) : MpBase( host )
,pinRetriggerMode( (int) RTM_PICKUP )
{
	// Register pins.
	initializePin( 0, pinVoiceReset );
	initializePin( 1, pinGate );
	initializePin( 2, pinRetriggerMode );
}

void EnvRetriggerModeAutoSet::subProcess( int bufferOffset, int sampleFrames )
{
	for( int s = sampleFrames; s > 0; --s )
	{
		if( --delayCount < 0 )
		{
			pinRetriggerMode.setValue( RTM_PICKUP, bufferOffset + sampleFrames - s );
			SET_PROCESS( &EnvRetriggerModeAutoSet::subProcessNothing );
			return;
		}
	}
}

void EnvRetriggerModeAutoSet::onSetPins()
{
	bool forcedReset = pinVoiceReset.isUpdated() && pinVoiceReset == 1.0f; // > 0.0f;

	if( forcedReset )
	{
		// envelope must reset to zero.
		pinRetriggerMode = RTM_RESTART;
		delayCount = 6;
		SET_PROCESS( &EnvRetriggerModeAutoSet::subProcess );
		resetSleepCounter();
	}
}

