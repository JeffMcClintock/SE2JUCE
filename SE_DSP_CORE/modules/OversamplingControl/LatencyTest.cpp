#include "./LatencyTest.h"

REGISTER_PLUGIN2 ( LatencyTest, L"SE Latency Test" );
REGISTER_PLUGIN2(LatencyTest2, L"SE Latency Test2");

LatencyTest::LatencyTest( )
{
	// Register pins.
	initializePin( pinSignalIn );
	initializePin( pinSignalOut );

	// Set processing method.
	setSubProcess(&LatencyTest::subProcess);
}

void LatencyTest::subProcess( int sampleFrames )
{
	// get pointers to in/output buffers.
	const float* signalIn	= getBuffer(pinSignalIn);
	float* signalOut= getBuffer(pinSignalOut);

	for( int s = sampleFrames; s > 0; --s )
	{
		*signalOut = *signalIn;

		// Increment buffer pointers.
		++signalIn;
		++signalOut;
	}
}

void LatencyTest::onSetPins()
{
	// Set state of output audio pins.
	pinSignalOut.setStreaming(pinSignalIn.isStreaming());
}
