#include "./DspFilters/PoleFilter.h"
#include "./DspFilters/Butterworth.h"
#include "../shared/FilterBase.h"

using namespace gmpi;

SE_DECLARE_INIT_STATIC_FILE(ButterworthLP2);

//#define ENABLE_DIAG_PINS

template <typename FilterClass>
class DspFilterBase2 : public FilterBase
{
protected:
	FilterClass filter[2];
	FilterClass* nextFilter = {};
	FilterClass* currrentFilter = {};
	float crossfade = -1.0f;
	float fade_inc = {};
	bool filterUpdated = {};
	std::vector<float> fadeBuffer;

	AudioInPin pinSignal;
	FloatInPin pinPitch;
	AudioOutPin pinOutput;
	IntInPin pinPoles;

#ifdef ENABLE_DIAG_PINS
	AudioOutPin pinDiag1;
	AudioOutPin pinDiag2;
	AudioOutPin pinDiag3;
	bool swapState = {};
#endif

public:
	// Support for FilterBase
	void StabilityCheck() override {}
	bool isFilterSettling() override
	{
		return !pinSignal.isStreaming();
	}
	AudioOutPin& getOutputPin() override
	{
		return pinOutput;
	}

	float normalisedPitch()
	{
		// limit pitch range to legal values.
		constexpr float minPitch = 0.00001f;// Almost zero
		constexpr float maxPitch = 0.499f;  // nyquist-ish

		return std::clamp(pinPitch.getValue() / getSampleRate(), minPitch, maxPitch);
	}

	virtual void setupFilter(FilterClass& f) = 0;

	template<bool isCrossfading>
	void subProcess(int sampleFrames)
	{
		doStabilityCheck();

		// get pointers to in/output buffers.
		float* signal = getBuffer(pinSignal);
		float* output = getBuffer(pinOutput);

#ifdef ENABLE_DIAG_PINS
		float* diag1 = getBuffer(pinDiag1);
		float* diag2 = getBuffer(pinDiag2);
		float* diag3 = getBuffer(pinDiag3);
		if (swapState)
			std::swap(diag1, diag2);
#endif

		if constexpr (isCrossfading)
		{
			if (crossfade == 1.0f)
			{
#ifdef ENABLE_DIAG_PINS
				swapState = !swapState;
#endif
				std::swap(nextFilter, currrentFilter);
				setSubProcess(&DspFilterBase2::subProcess<false>); // done crossfading
				initSettling(); // reenable sleep monitoring
				subProcess<false>(sampleFrames);
				return;
			}
		}
		else
		{
			if (filterUpdated)
			{
				filterUpdated = false;
				crossfade = -1.0f;
				setupFilter(*nextFilter);
				nextFilter->reset(); // clear any audio out that might cause instability

				setSubProcess(&DspFilterBase2::subProcess<true>); // start crossfading
				subProcess<true>(sampleFrames);
				return;
			}

#ifdef ENABLE_DIAG_PINS
			std::fill(diag1, diag1 + sampleFrames, -0.2f);
			std::fill(diag2, diag2 + sampleFrames, -0.2f);
			std::fill(diag3, diag3 + sampleFrames, -0.2f);
#endif
		}

		// This design processes in-place.
		// Copy input to output buffer.
		std::copy(signal, signal + sampleFrames, output);

		currrentFilter->process(sampleFrames, &output);

		if constexpr (isCrossfading)
		{
			float* dest = (float*)fadeBuffer.data();
			std::copy(signal, signal + sampleFrames, dest);

			// fade up the signal into the new filter to avoid spikes
			{
				float* signal = (float*)fadeBuffer.data();
				auto lcrossfade = crossfade;
				for (int i = 0; i < sampleFrames; ++i)
				{
					lcrossfade = std::min(1.0f, lcrossfade + fade_inc);
					const auto fadeUp = std::min(1.0f, lcrossfade + 1.0f);

					*signal++ *= fadeUp;
				}
			}
			
			nextFilter->process(sampleFrames, &dest);

			for (int i = 0; i < sampleFrames; ++i)
			{
				crossfade = std::min(1.0f, crossfade + fade_inc);
				const auto clampedCrossfade = std::max(0.0f, crossfade);

#ifdef ENABLE_DIAG_PINS
				*diag1++ = clampedCrossfade * fadeBuffer[i];
				*diag2++ = (1.f - clampedCrossfade) * output[i];
//				*diag3++ = crossfade;
				*diag3++ = fadeBuffer[i];	// raw output of new filter, to show glitches
#endif

				output[i] += clampedCrossfade * (fadeBuffer[i] - output[i]);
			}
		}
#ifdef ENABLE_DIAG_PINS
		else
		{
			for (int i = 0; i < sampleFrames; ++i)
			{
				*diag1++ = 0.0f;
				*diag2++ = output[i];
				*diag3++ = 0.0f;
			}
		}
#endif
	}

	int32_t MP_STDCALL open() override
	{
		fadeBuffer.assign(getBlockSize(), 0.0f);
		fade_inc = 1.0f / (0.05f * getSampleRate());
		return FilterBase::open();
	}

	void onGraphStart() override
	{
		FilterBase::onGraphStart();

		setupFilter(filter[0]);
		setupFilter(filter[1]);

		nextFilter = &filter[0];
		currrentFilter = &filter[1];

		setSubProcess(&DspFilterBase2::subProcess<false>);

		// Set state of output audio pins.
		pinOutput.setStreaming(true);
	}

	void onSetPins() override
	{
		// Set processing method.
		if(pinPitch.isUpdated() || pinPoles.isUpdated())
			filterUpdated = true;

		// updating any input potentially changes output signal.
		pinOutput.setStreaming(true);
		setSubProcess(&DspFilterBase2::subProcess<false>);

		initSettling();
	}
};

template <typename FilterClass>
class ButterworthBase2 : public DspFilterBase2<FilterClass>
{
public:
	ButterworthBase2()
	{
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinSignal);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinPitch);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinPoles);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinOutput);

#ifdef ENABLE_DIAG_PINS
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinDiag1);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinDiag2);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinDiag3);
#endif
	}

	void setupFilter(FilterClass &f) override
	{
		f.setup(DspFilterBase2<FilterClass>::pinPoles, 1.0, DspFilterBase2<FilterClass>::normalisedPitch());
	}

	// Support for FilterBase
	void StabilityCheck() override {}
};

class ButterworthLp2 : public ButterworthBase2< Dsp::SimpleFilter<Dsp::Butterworth::LowPass <12>, 1> > // 12 poles max, 1 channel.
{
};

class ButterworthHp2 : public ButterworthBase2< Dsp::SimpleFilter<Dsp::Butterworth::HighPass <12>, 1> > // 12 poles max, 1 channel.
{
};

template <typename FilterClass>
class ButterworthBandBase2 : public DspFilterBase2<FilterClass>
{
	FloatInPin pinWidth;

public:
	ButterworthBandBase2()
	{
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinSignal);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinPitch);
		MpBase2::initializePin(pinWidth);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinPoles);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinOutput);

#ifdef ENABLE_DIAG_PINS
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinDiag1);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinDiag2);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinDiag3);
#endif
	}

	void setupFilter(FilterClass& f) override
	{
		// limit pitch range to legal values.
		float normalisedWidth = (std::max)(0.01f, pinWidth.getValue()) / DspFilterBase2<FilterClass>::getSampleRate();
		const float maxPitch = 0.499f;
		if (normalisedWidth > maxPitch)
		{
			normalisedWidth = maxPitch;
		}

		f.setup(
			DspFilterBase2<FilterClass>::pinPoles.getValue()
			, 1.0
			, DspFilterBase2<FilterClass>::normalisedPitch()
			, normalisedWidth
		);
	}

	void onSetPins() override
	{
		if (pinWidth.isUpdated())
			DspFilterBase2<FilterClass>::filterUpdated = true;

		DspFilterBase2<FilterClass>::onSetPins();
	}
};

class ButterworthBp2 : public ButterworthBandBase2< Dsp::SimpleFilter<Dsp::Butterworth::BandPass <12>, 1> > // 12 poles max, 1 channel.
{
};

class ButterworthBr2 : public ButterworthBandBase2< Dsp::SimpleFilter<Dsp::Butterworth::BandStop <12>, 1> > // 12 poles max, 1 channel.
{
};

class ButterworthBandShelf2 : public DspFilterBase2< Dsp::SimpleFilter<Dsp::Butterworth::BandShelf <12>, 1> >
{
	FloatInPin pinWidth;
	FloatInPin pinGain;

public:
	ButterworthBandShelf2()
	{
		initializePin(pinSignal);
		initializePin(pinPitch);
		initializePin(pinWidth);
		initializePin(pinGain);
		initializePin(pinPoles);
		initializePin(pinOutput);
	}

	void setupFilter(Dsp::SimpleFilter<Dsp::Butterworth::BandShelf <12>, 1>& f) override
	{
		// limit pitch range to legal values.
		float normalisedWidth = (std::max)(0.01f, pinWidth.getValue()) / getSampleRate();
		const float maxPitch = 0.499f;
		if (normalisedWidth > maxPitch)
		{
			normalisedWidth = maxPitch;
		}

		f.setup(
			pinPoles.getValue(),
			1.0,
			normalisedPitch(),
			normalisedWidth,
			pinGain.getValue()
		);
	}

	void onSetPins() override
	{
		if (pinWidth.isUpdated() || pinGain.isUpdated())
			filterUpdated = true;

		DspFilterBase2< Dsp::SimpleFilter<Dsp::Butterworth::BandShelf <12>, 1> >::onSetPins();
	}
};

template <typename FilterClass>
class ButterworthShelf2 : public DspFilterBase2< FilterClass >
{
	FloatInPin pinGain;

public:
	ButterworthShelf2()
	{
		DspFilterBase2<FilterClass>::initializePin(DspFilterBase2<FilterClass>::pinSignal);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinPitch);
		MpBase2::initializePin(pinGain);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinPoles);
		MpBase2::initializePin(DspFilterBase2<FilterClass>::pinOutput);
	}

	void setupFilter(FilterClass& f) override
	{
		//?        assert(pinGain.getValue() <= 0.0f); // Shelving filters can't have positive gain.
		f.setup(DspFilterBase2< FilterClass >::pinPoles, 1.0, DspFilterBase2<FilterClass>::normalisedPitch(), pinGain.getValue());
	}
	void onSetPins() override
	{
		if (pinGain.isUpdated())
			DspFilterBase2<FilterClass>::filterUpdated = true;

		DspFilterBase2< FilterClass >::onSetPins();
	}
};

class ButterworthLowShelf2 : public ButterworthShelf2< Dsp::SimpleFilter<Dsp::Butterworth::LowShelf <12>, 1> > // 12 poles max, 1 channel.
{
};

class ButterworthHighShelf2 : public ButterworthShelf2< Dsp::SimpleFilter<Dsp::Butterworth::HighShelf <12>, 1> > // 12 poles max, 1 channel.
{
};

namespace
{
	auto r1 = Register<ButterworthLp2>::withId(L"SE Butterworth LP2");
	auto r2 = Register<ButterworthHp2>::withId(L"SE Butterworth HP2");
	auto r3 = Register<ButterworthBp2>::withId(L"SE Butterworth BP2");
	auto r4 = Register<ButterworthBr2>::withId(L"SE Butterworth BR2");
	auto r5 = Register<ButterworthLowShelf2>::withId(L"SE Butterworth LS2");
	auto r6 = Register<ButterworthHighShelf2>::withId(L"SE Butterworth HS2");
	auto r7 = Register<ButterworthBandShelf2>::withId(L"SE Butterworth BS2");
}

