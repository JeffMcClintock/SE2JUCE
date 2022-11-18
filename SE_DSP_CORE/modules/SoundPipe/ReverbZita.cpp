#include "../se_sdk3/mp_sdk_audio.h"
extern "C"
{
	#include "./h/soundpipe.h"
}

using namespace gmpi;

class ReverbZita : public MpBase2
{
	AudioInPin pinLInmono;
	AudioInPin pinRIn;
	AudioOutPin pinLOut;
	AudioOutPin pinROut;

	static const int argsCount = 11;
	FloatInPin argsPins[argsCount];

	sp_zitarev* reverbInfo = nullptr;
	sp_data soundPipeData;

public:
	ReverbZita() : soundPipeData { nullptr, 44100, 2, 0, 0, "", 0 }
	{
		initializePin( pinLInmono );
		initializePin( pinRIn );
		initializePin( pinLOut );
		initializePin( pinROut );

		for( auto& pin : argsPins)
			initializePin(pin);
	}

	~ReverbZita()
	{
		if (reverbInfo)
			sp_zitarev_destroy(&reverbInfo);
	}

	int32_t MP_STDCALL open() override
	{
		sp_zitarev_create(&reverbInfo);
		sp_zitarev_init(&soundPipeData, reverbInfo);

		return MpBase2::open();
	}

	void subProcess( int sampleFrames )
	{
		// get pointers to in/output buffers.
		auto lInmono = getBuffer(pinLInmono);
		auto rIn = getBuffer(pinRIn);
		auto lOut = getBuffer(pinLOut);
		auto rOut = getBuffer(pinROut);

		soundPipeData.len = sampleFrames;

		for( int s = sampleFrames; s > 0; --s )
		{
			sp_zitarev_compute(&soundPipeData, reverbInfo, lInmono, rIn, lOut, rOut);

			// Increment buffer pointers.
			++lInmono;
			++rIn;
			++lOut;
			++rOut;
		}
	}

	void onSetPins() override
	{
		bool updateArgs = false;

		for (auto& pin : argsPins)
			updateArgs |= pin.isUpdated();

		if(updateArgs)
		{
			soundPipeData.sr = static_cast<int>(getSampleRate());

			*reverbInfo->in_delay = argsPins[0].getValue();
			*reverbInfo->lf_x = argsPins[1].getValue();
			*reverbInfo->rt60_low = argsPins[2].getValue();
			*reverbInfo->rt60_mid = argsPins[3].getValue();
			*reverbInfo->hf_damping = argsPins[4].getValue();
			*reverbInfo->eq1_freq = argsPins[5].getValue();
			*reverbInfo->eq1_level = argsPins[6].getValue();
			*reverbInfo->eq2_freq = argsPins[7].getValue();
			*reverbInfo->eq2_level = argsPins[8].getValue();
			*reverbInfo->mix = argsPins[9].getValue();
			*reverbInfo->level = argsPins[10].getValue();
		}

		// Set state of output audio pins.
		pinLOut.setStreaming(true);
		pinROut.setStreaming(true);

		// Set processing method.
		setSubProcess(&ReverbZita::subProcess);
	}
};

namespace
{
	auto r = Register<ReverbZita>::withId(L"SP Reverb Zita");
}
