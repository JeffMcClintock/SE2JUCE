#include "./StreamingState.h"

REGISTER_PLUGIN ( StreamingState, L"SE Streaming State" );

StreamingState::StreamingState( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( 0, pinSignalIn );
	initializePin( 1, pinSignalOut );

	pinSignalOut.setTransitionTime(1.0f);
}

void StreamingState::subProcess( int bufferOffset, int sampleFrames )
{
	bool canSleep = true;
	pinSignalOut.subProcess(bufferOffset, sampleFrames, canSleep);
}

void StreamingState::onSetPins()
{
	if (pinSignalIn.isUpdated())
	{
		if (pinSignalIn.isStreaming())
		{
			pinSignalOut.setValueInstant(1.0f);
		}
		else
		{
			pinSignalOut.pulse(0.5f);
		}
	}

	// Set processing method.
	SET_PROCESS(&StreamingState::subProcess);
}

