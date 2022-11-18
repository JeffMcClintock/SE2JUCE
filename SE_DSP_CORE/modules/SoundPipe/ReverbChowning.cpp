#include "../shared/FilterBase.h"
extern "C"
{
	#include "soundpipe.h"
}

using namespace gmpi;

class ReverbChowning : public MpBase2 //  TODO: support stereo output in FilterBase
{
	AudioInPin pinLIn;
	AudioOutPin pinLOut;

	sp_jcrev* reverbInfo = nullptr;
	sp_data soundPipeData;

public:
	ReverbChowning() : soundPipeData { nullptr, 44100, 2, 0, 0, "", 0 }
	{
		initializePin( pinLIn );
		initializePin( pinLOut );
	}

	~ReverbChowning()
	{
		if (reverbInfo)
			sp_jcrev_destroy(&reverbInfo);
	}

	int32_t MP_STDCALL open() override
	{
		sp_jcrev_create(&reverbInfo);
		sp_jcrev_init(&soundPipeData, reverbInfo);

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
		auto lOut = getBuffer(pinLOut);

		soundPipeData.len = sampleFrames;

		for( int s = sampleFrames; s > 0; --s )
		{
			sp_jcrev_compute(&soundPipeData, reverbInfo, lInmono, lOut);

			// Increment buffer pointers.
			++lInmono;
			++lOut;
		}
	}

	void onSetPins() override
	{
		// Set state of output audio pins.
		pinLOut.setStreaming(true);

		// Set processing method.
		setSubProcess(&ReverbChowning::subProcess);

//		initSettling(); // must be last.
	}
};

namespace
{
	auto r = Register<ReverbChowning>::withId(L"SP Reverb JC");
}
