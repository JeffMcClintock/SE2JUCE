#include "./TestSem.h"

REGISTER_PLUGIN2 ( TestSem, L"SE Test SEM" );

TestSem::TestSem( )
{
	// Register pins.
	initializePin( pinCaptureDataA );
	initializePin( pinCaptureDataB );
	initializePin( pinSignalA );
	initializePin( pinSignalB );
	initializePin( pinVoiceActive );
	initializePin( pinpolydetect );
}

void TestSem::subProcess( int sampleFrames )
{
	// get pointers to in/output buffers.
	const float* signalA = getBuffer(pinSignalA);
	const float* signalB = getBuffer(pinSignalB);

	for( int s = sampleFrames; s > 0; --s )
	{
		// TODO: Signal processing goes here.

		// Increment buffer pointers.
		++signalA;
		++signalB;
	}
}

void TestSem::onSetPins(void)
{
	// Check which pins are updated.
	if( pinSignalA.isStreaming() )
	{
	}
	if( pinSignalB.isStreaming() )
	{
	}
	if( pinVoiceActive.isUpdated() )
	{
	}

	// Set processing method.
	setSubProcess(&TestSem::subProcess);

	// Set sleep mode (optional).
	// setSleep(false);
}

