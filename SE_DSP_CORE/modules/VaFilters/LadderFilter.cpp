#include "./LadderFilter.h"

REGISTER_PLUGIN2(LadderFilter_test, L"SE Moog Filter test");
REGISTER_PLUGIN2(LadderFilter, L"SE Moog Filter");


LadderFilter::LadderFilter()
{
	// Register pins.
	initializePin(pinSignal);
	initializePin(pinPitch);
	initializePin(pinResonance);
	initializePin(pinSaturation);
	initializePin(pinMode);
	initializePin(pinSaturator);
	initializePin(pinOutput);
}

int32_t LadderFilter::open()
{
	filter.initialize(getSampleRate());
	return MpBase2::open();
}

void LadderFilter::onSetPins()
{
	// Check which pins are updated.
	if( pinSignal.isStreaming() )
	{
	}
	if( pinPitch.isStreaming() )
	{
	}
	if( pinResonance.isStreaming() )
	{
	}
	if( pinSaturation.isUpdated() )
	{
	}
	if( pinMode.isUpdated() )
	{
	}
	if( pinSaturator.isUpdated() )
	{
	}

	// Set state of output audio pins.
	pinOutput.setStreaming(true);

	// Set processing method.
	SET_PROCESS2(&LadderFilter::subProcess);

	// Set sleep mode (optional).
	// setSleep(false);
	
	initSettling(); // must be last.
}

void LadderFilter::StabilityCheck()
{
	if (!filter.isStable())
	{
		filter.initialize(getSampleRate());
	}
}

LadderFilter_test::LadderFilter_test()
{
	// Register pins.
	initializePin(pinSignal);
	initializePin(pinPitch);
	initializePin(pinResonance);
	initializePin(pinSaturation);
	initializePin(pinMode);
	initializePin(pinSaturator);
	initializePin(pinOutput);
}

int32_t LadderFilter_test::open()
{
	filter.initialize(getSampleRate());
	return MpBase2::open();
}

void LadderFilter_test::subProcess(int sampleFrames)
{
	// get pointers to in/output buffers.
	float* signal = getBuffer(pinSignal);
	float* pitch = getBuffer(pinPitch);
	float* resonance = getBuffer(pinResonance);
	float* output = getBuffer(pinOutput);

	float hz = 440.f * powf(2.f, ( 10.0f * *pitch ) - 5.f);
	filter.calculateCoeffsMoog(hz * 50.0f / getSampleRate(), *resonance * 10.0f);

	for( int s = sampleFrames; s > 0; --s )
	{
		// TODO: Signal processing goes here.
		*output = filter.doMoogLPF(*signal);

		// Increment buffer pointers.
		++signal;
		++pitch;
		++resonance;
		++output;
	}
}

void LadderFilter_test::onSetPins()
{
	// Set state of output audio pins.
	pinOutput.setStreaming(true);

	// Set processing method.
	SET_PROCESS2(&LadderFilter_test::subProcess);
}
