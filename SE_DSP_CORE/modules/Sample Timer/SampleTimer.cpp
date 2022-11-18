#include "./SampleTimer.h"

REGISTER_PLUGIN2 ( SampleTimer, L"SE Sample Timer" );

SampleTimer::SampleTimer( ) 
	: outValue_(0), timer_(0)
{
	// Register pins.
	initializePin(pinTimeIn);
	initializePin(  pinSignalOut );
}

void SampleTimer::subProcess( int sampleFrames )
{
	// get pointers to in/output buffers.
	float* signalOut = getBuffer(pinSignalOut);

	for( int s = sampleFrames; s > 0; --s )
	{
		if( timer_++ == pinTimeIn )
		{
			outValue_ = 1;
			pinSignalOut.setUpdated(this->getBlockPosition() + sampleFrames - s);
		}
		*signalOut++ = outValue_;
	}
}

void SampleTimer::onSetPins(void)
{
	// Set state of output audio pins.
	pinSignalOut.setStreaming(false);

	// Set processing method.
	SET_PROCESS2(&SampleTimer::subProcess);

	// Set sleep mode (optional).
	setSleep(false);
}

