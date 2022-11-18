#include "./Impulse.h"

REGISTER_PLUGIN ( Impulse, L"SE Impulse" );
SE_DECLARE_INIT_STATIC_FILE(Impulse);

Impulse::Impulse( IMpUnknown* host ) : MpBase( host )
	,Triggerstate_(false)
{
	// Register pins.
	initializePin( 0, pinTrigger );
	initializePin( 1, pinAudioOut );
}

void Impulse::subProcess( int bufferOffset, int sampleFrames )
{
	// get pointers to in/output buffers.
	float* trigger	= bufferOffset + pinTrigger.getBuffer();
	float* audioOut	= bufferOffset + pinAudioOut.getBuffer();

	for( int s = sampleFrames; s > 0; --s )
	{
		bool newTriggerstate = *trigger > 0.0f;

		*audioOut = 0.0f;
		if( newTriggerstate != Triggerstate_ )
		{
			Triggerstate_ = newTriggerstate;
			if( Triggerstate_ )
			{
				*audioOut = 1.0f;
			}
		}
		++audioOut;
		++trigger;
	}
}

void Impulse::onSetPins()
{
	// Set state of output audio pins.
	pinAudioOut.setStreaming(true);

	// Set processing method.
	SET_PROCESS(&Impulse::subProcess);
}

