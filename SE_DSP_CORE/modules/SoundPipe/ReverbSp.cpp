#include "../shared/FilterBase.h"
extern "C"
{
	#include "soundpipe.h"
}

using namespace gmpi;

class ReverbSp : public MpBase2 //  TODO: support stereo output in FilterBase
{
	AudioInPin pinLIn;
	AudioInPin pinRIn;
	AudioOutPin pinLOut;
	AudioOutPin pinROut;
	FloatInPin pinFeedBack;
	FloatInPin pinLowPass;

	sp_revsc* reverbInfo = nullptr;
	sp_data soundPipeData;

public:
	ReverbSp() : soundPipeData { nullptr, 44100, 2, 0, 0, "", 0 }
	{
		initializePin( pinLIn );
		initializePin( pinRIn );
		initializePin( pinLOut );
		initializePin( pinROut );
		initializePin(pinFeedBack);
		initializePin(pinLowPass);
	}

	~ReverbSp()
	{
		if (reverbInfo)
			sp_revsc_destroy(&reverbInfo);
	}

	int32_t MP_STDCALL open() override
	{
		sp_revsc_create(&reverbInfo);
		sp_revsc_init(&soundPipeData, reverbInfo);

		return MpBase2::open();
	}

	/* todo
	// Support for FilterBase
	// This is called to determin when your filter is settling. Typically you need to check if all your input pins are quiet (not streaming).
	virtual bool isFilterSettling() override
	{
		return !pinLIn.isStreaming() && !pinRIn.isStreaming(); // <-example code. Place your code here.
	}

	// This allows the base class to monitor the filter's output signal, provide an audio output pin.
	virtual AudioOutPin& getOutputPin() override
	{
		return pinLOut; // <-example code. Place your code here.
	}
	*/

	void subProcess( int sampleFrames )
	{
//		doStabilityCheck();

		// get pointers to in/output buffers.
		auto lInmono = getBuffer(pinLIn);
		auto rIn = getBuffer(pinRIn);
		auto lOut = getBuffer(pinLOut);
		auto rOut = getBuffer(pinROut);

		soundPipeData.len = sampleFrames;

		for( int s = sampleFrames; s > 0; --s )
		{
			sp_revsc_compute(&soundPipeData, reverbInfo, lInmono, rIn, lOut, rOut);

			// Increment buffer pointers.
			++lInmono;
			++rIn;
			++lOut;
			++rOut;
		}
	}

	virtual void onSetPins() override
	{
		if(pinFeedBack.isUpdated() || pinLowPass.isUpdated())
		{
			soundPipeData.sr = static_cast<int>(getSampleRate());

			reverbInfo->feedback = pinFeedBack;
			reverbInfo->lpfreq = pinLowPass;
		}

		// Set state of output audio pins.
		pinLOut.setStreaming(true);
		pinROut.setStreaming(true);

		// Set processing method.
		setSubProcess(&ReverbSp::subProcess);

//		initSettling(); // must be last.
	}
};

namespace
{
	auto r = Register<ReverbSp>::withId(L"SP Reverb SP");
}
