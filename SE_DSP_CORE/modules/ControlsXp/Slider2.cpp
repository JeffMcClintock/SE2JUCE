#include "./Slider2.h"

REGISTER_PLUGIN2 ( Slider2, L"SE Slider2" );

Slider2::Slider2()
{
	// Register pins.
	initializePin( pinValueIn);
	initializePin( pinValueOut );
}

void Slider2::subProcess( int sampleFrames )
{
	bool canSleep = true;
	pinValueOut.subProcess(getBlockPosition(), sampleFrames, canSleep);
}

void Slider2::onSetPins()
{
	pinValueOut = 0.1f * pinValueIn; // 1 V = 0.1 audio.

	// Set processing method.
	SET_PROCESS2(&Slider2::subProcess);
}

