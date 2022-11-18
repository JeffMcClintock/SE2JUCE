#include "./SampleRate.h"

REGISTER_PLUGIN ( SampleRate, L"SE Sample Rate" );

SampleRate::SampleRate( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( pinSampleRate );
}

void SampleRate::onGraphStart()
{
	MpBase::onGraphStart();
	pinSampleRate = getSampleRate();
}

