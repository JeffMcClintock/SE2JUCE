#include "./NoteExpression.h"

using namespace gmpi;

SE_DECLARE_INIT_STATIC_FILE(NoteExpression);

class NoteExpression2 final : public MpBase2
{
	FloatInPin pinVolume;
	FloatInPin pinPan;
	FloatInPin pinBrightness;
	FloatInPin pinPressure;
	FloatOutPin pinVolumeOut;
	FloatOutPin pinPanOut;
	FloatOutPin pinBrightnessOut;
	FloatOutPin pinPressureOut;

public:
	NoteExpression2()
	{
		initializePin( pinVolume );
		initializePin( pinPan );
		initializePin( pinBrightness );
		initializePin( pinPressure );
		initializePin( pinVolumeOut );
		initializePin( pinPanOut );
		initializePin( pinBrightnessOut );
		initializePin( pinPressureOut );
	}

	void onSetPins() override
	{
		// Check which pins are updated.
		if( pinVolume.isUpdated() )
		{
			pinVolumeOut = pinVolume;
		}
		if( pinPan.isUpdated() )
		{
			pinPanOut = pinPan;
		}
		if( pinBrightness.isUpdated() )
		{
			pinBrightnessOut = pinBrightness;
		}
		if( pinPressure.isUpdated() )
		{
			pinPressureOut = pinPressure;
		}
	}
};

namespace
{
	auto r = Register<NoteExpression2>::withId(L"SE Note Expression2");
}


REGISTER_PLUGIN2 ( NoteExpression, L"SE Note Expression" );

NoteExpression::NoteExpression()
{
	for (int i = 0; i < 8; ++i)
	{
		initializePin(i, inPins[i]);
	}
}

int32_t NoteExpression::open()
{
	MpBase2::open();

	float transitionSmoothing = getSampleRate() * 0.005f; // 5 ms smoothing.
	for (int i = 0; i < 8; ++i)
	{
		int j = i + 8;
		initializePin(j, outPins[i]);
		outPins[i].setCurveType(SmartAudioPin::Curved);
		outPins[i].setTransitionTime(transitionSmoothing);
	}

	return gmpi::MP_OK;
}

void NoteExpression::subProcess( int sampleFrames )
{
	int32_t bufferOffset = getBlockPosition();
	bool canSleep = true;
	for (int i = 0; i < 8; ++i)
	{
		outPins[i].subProcess(bufferOffset, sampleFrames, canSleep);
	}
}

void NoteExpression::onSetPins()
{
	for (int i = 0; i < 8; ++i)
	{
		if (inPins[i].isUpdated())
		{
			outPins[i] = 0.1f * inPins[i];
//			_RPTN(0, "                                 NoteExpression: %d %f\n", i, inPins[i].getValue());
		}
	}

	// Set processing method.
	SET_PROCESS2(&NoteExpression::subProcess);

	// Set sleep mode (optional).
	// setSleep(false);
}

