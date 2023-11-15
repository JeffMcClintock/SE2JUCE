#pragma once

#include "./DspFilters/PoleFilter.h"
#include "./DspFilters/Butterworth.h"
#include "../shared/FilterBase.h"

template <typename FilterClass>
class DspFilterBase : public FilterBase
{
protected:
	FilterClass filter;

	AudioInPin pinSignal;
	FloatInPin pinPitch;
	AudioOutPin pinOutput;
	IntInPin pinPoles;

public:
	// Support for FilterBase
	void StabilityCheck() override
	{
	}
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
		const float minHz = 0.0001f; // 0.1 Hz
		const float maxPitch = 0.499f;  // nyquist-ish

		return (std::min)(maxPitch, (std::max)(minHz, pinPitch.getValue()) / getSampleRate());
	}

	virtual void setupFilter() = 0;
	void onSetPins() override
	{
		setupFilter();

		// Set state of output audio pins.
		pinOutput.setStreaming(true);

		// Set processing method.
		setSubProcess(&DspFilterBase::subProcess);

		initSettling();
	}

	void subProcess(int sampleFrames)
	{
		doStabilityCheck();

		// get pointers to in/output buffers.
		float* signal = getBuffer(pinSignal);
		float* output = getBuffer(pinOutput);

		// This design processes in-place.
		// Copy input to output buffer.
		auto o = output;
		for (int i = 0; i < sampleFrames; ++i)
		{
			*o++ = *signal++;
		}

		filter.process(sampleFrames, &output);
	}
};

template <typename FilterClass>
class ButterworthBase : public DspFilterBase<FilterClass>
{
public:
	ButterworthBase()
	{
        MpBase2::initializePin(DspFilterBase<FilterClass>::pinSignal);
		MpBase2::initializePin(DspFilterBase<FilterClass>::pinPitch);
		MpBase2::initializePin(DspFilterBase<FilterClass>::pinPoles);
		MpBase2::initializePin(DspFilterBase<FilterClass>::pinOutput);
	}

	void setupFilter() override
	{
		DspFilterBase<FilterClass>::filter.setup(DspFilterBase<FilterClass>::pinPoles, 1.0, DspFilterBase<FilterClass>::normalisedPitch());
	}

	// Support for FilterBase
	void StabilityCheck() override {}
};

class ButterworthLp : public ButterworthBase< Dsp::SimpleFilter<Dsp::Butterworth::LowPass <12>, 1> > // 12 poles max, 1 channel.
{
};

class ButterworthHp : public ButterworthBase< Dsp::SimpleFilter<Dsp::Butterworth::HighPass <12>, 1> > // 12 poles max, 1 channel.
{
};

template <typename FilterClass>
class ButterworthBandBase : public DspFilterBase<FilterClass>
{
	FloatInPin pinWidth;

public:
	ButterworthBandBase()
	{
		MpBase2::initializePin(DspFilterBase<FilterClass>::pinSignal);
		MpBase2::initializePin(DspFilterBase<FilterClass>::pinPitch);
		MpBase2::initializePin(pinWidth);
		MpBase2::initializePin(DspFilterBase<FilterClass>::pinPoles);
		MpBase2::initializePin(DspFilterBase<FilterClass>::pinOutput);
	}

	void setupFilter() override
	{
		// limit pitch range to legal values.
		float normalisedWidth = (std::max)(0.01f, pinWidth.getValue()) / DspFilterBase<FilterClass>::getSampleRate();
		const float maxPitch = 0.499f;
		if (normalisedWidth > maxPitch)
		{
			normalisedWidth = maxPitch;
		}

		DspFilterBase<FilterClass>::filter.setup(
			DspFilterBase<FilterClass>::pinPoles.getValue()
			, 1.0
			, DspFilterBase<FilterClass>::normalisedPitch()
			, normalisedWidth
		);
	}
};

/* made converter instead
// Butterworth with width specified in Octaves (not Hz)
template <typename FilterClass>
class ButterworthBandBaseOct : public DspFilterBase<FilterClass>
{
	FloatInPin pinWidth;

public:
	ButterworthBandBaseOct()
	{
		initializePin(pinSignal);
		initializePin(pinPitch);
		initializePin(pinWidth);
		initializePin(pinPoles);
		initializePin(pinOutput);
	}

	void setupFilter() override
	{
		const float minOctave = 0.001f;
		const float minFreq = 0.001f; // 10Hz
		const float maxFreq = 0.499f * getSampleRate();

		// Width is in Octaves.
		auto centerFreqHz = (std::min)(maxFreq, (std::max)(minFreq, pinPitch.getValue()));
		auto octaves = (std::max)(minOctave, pinWidth.getValue());

		auto ratio = pow(2.0f, octaves);
		auto widthHz = 2.0f * (centerFreqHz * (ratio - 1)) / (ratio + 1);

		filter.setup(pinPoles, getSampleRate(), centerFreqHz, widthHz);
	}

	// Support for FilterBase
	void StabilityCheck() override {}
};

class ButterworthBpOct : public ButterworthBandBaseOct< Dsp::SimpleFilter<Dsp::Butterworth::BandPass <12>, 1> > // 12 poles max, 1 channel.
{
};

class ButterworthBrOct : public ButterworthBandBaseOct< Dsp::SimpleFilter<Dsp::Butterworth::BandStop <12>, 1> > // 12 poles max, 1 channel.
{
};
*/

class ButterworthBp : public ButterworthBandBase< Dsp::SimpleFilter<Dsp::Butterworth::BandPass <12>, 1> > // 12 poles max, 1 channel.
{
};

class ButterworthBr : public ButterworthBandBase< Dsp::SimpleFilter<Dsp::Butterworth::BandStop <12>, 1> > // 12 poles max, 1 channel.
{
};

class ButterworthBandShelf : public DspFilterBase< Dsp::SimpleFilter<Dsp::Butterworth::BandShelf <12>, 1> >
{
	FloatInPin pinWidth;
	FloatInPin pinGain;

public:
	ButterworthBandShelf()
	{
		initializePin(pinSignal);
		initializePin(pinPitch);
		initializePin(pinWidth);
		initializePin(pinGain);
		initializePin(pinPoles);
		initializePin(pinOutput);
	}

	void setupFilter() override
	{
		// limit pitch range to legal values.
		float normalisedWidth = (std::max)(0.01f, pinWidth.getValue()) / getSampleRate();
		const float maxPitch = 0.499f;
		if (normalisedWidth > maxPitch)
		{
			normalisedWidth = maxPitch;
		}

//		float safeGain = (std::min)(300.0f, (std::max)(0.01f, pinGain.getValue()));

		filter.setup(
			pinPoles.getValue(),
			1.0,
			normalisedPitch(),
			normalisedWidth,
			pinGain.getValue()
		);
	}
};

template <typename FilterClass>
class ButterworthShelf : public DspFilterBase< FilterClass >
{
	FloatInPin pinGain;

public:
	ButterworthShelf()
	{
		DspFilterBase<FilterClass>::initializePin(DspFilterBase<FilterClass>::pinSignal);
		MpBase2::initializePin(DspFilterBase<FilterClass>::pinPitch);
		MpBase2::initializePin(pinGain);
		MpBase2::initializePin(DspFilterBase<FilterClass>::pinPoles);
		MpBase2::initializePin(DspFilterBase<FilterClass>::pinOutput);
	}

	void setupFilter() override
	{
//?        assert(pinGain.getValue() <= 0.0f); // Shelving filters can't have positive gain.
		DspFilterBase<FilterClass>::filter.setup(DspFilterBase< FilterClass >::pinPoles, 1.0, DspFilterBase<FilterClass>::normalisedPitch(), pinGain.getValue());
	}
};

class ButterworthLowShelf : public ButterworthShelf< Dsp::SimpleFilter<Dsp::Butterworth::LowShelf <12>, 1> > // 12 poles max, 1 channel.
{
};

class ButterworthHighShelf : public ButterworthShelf< Dsp::SimpleFilter<Dsp::Butterworth::HighShelf <12>, 1> > // 12 poles max, 1 channel.
{
};

/*
class EllipticLp : public DspFilterBase< Dsp::SimpleFilter<Dsp::Elliptic::LowPass <12>, 1> > // 12 poles max, 1 channel.
{
	FloatInPin pinRipple;
	FloatInPin pinRolloff;

public:
	EllipticLp()
	{
		initializePin(pinSignal);
		initializePin(pinPitch);
		initializePin(pinPoles);
		initializePin(pinRipple);
		initializePin(pinRolloff);
		initializePin(pinOutput);
	}

	void setupFilter() override
	{
		filter.setup(pinPoles, 1.0, normalisedPitch(), pinRipple, pinRolloff);
	}
};


class EllipticHp : public DspFilterBase< Dsp::SimpleFilter<Dsp::Elliptic::HighPass <12>, 1> > // 12 poles max, 1 channel.
{
	FloatInPin pinRipple;
	FloatInPin pinRolloff;

public:
	EllipticHp()
	{
		initializePin(pinSignal);
		initializePin(pinPitch);
		initializePin(pinPoles);
		initializePin(pinRipple);
		initializePin(pinRolloff);
		initializePin(pinOutput);
	}

	void setupFilter() override
	{
		filter.setup(pinPoles, 1.0, normalisedPitch(), pinRipple, pinRolloff);
	}
};
*/
