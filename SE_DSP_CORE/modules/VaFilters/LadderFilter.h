#ifndef LADDERFILTER_H_INCLUDED
#define LADDERFILTER_H_INCLUDED

#include "../shared/FilterBase.h"
#include "VAFilters/MoogLadderFilter.h"
#include "VAFilters/TPTMoogLadderFilter.h"

// optimised version.
class LadderFilter : public FilterBase
{
//	CMoogLadderFilter filter;
	CTPTMoogLadderFilter filter;

public:
	LadderFilter( );

	// FilterBase support
	void StabilityCheck() override;
	bool isFilterSettling() override
	{
		return !pinSignal.isStreaming();
	}

	AudioOutPin& getOutputPin() override
	{
		return pinOutput;
	}

//	template< class PitchModulationPolicy, class ResonanceModulationPolicy, class FilterModePolicy >
	void subProcess(int sampleFrames)
	{
		doStabilityCheck(); // must be first

		// get pointers to in/output buffers.
		float* signal = getBuffer(pinSignal);
		float* pitch = getBuffer(pinPitch);
		float* resonance = getBuffer(pinResonance);
		float* output = getBuffer(pinOutput);

		float hz = 440.f * powf(2.f, ( 10.0f * *pitch ) - 5.f);
		hz = (std::min)(hz, 21000.f);

		filter.calculateTPTCoeffs(hz, *resonance * 25);

		for( int s = sampleFrames; s > 0; --s )
		{
			// TODO: Signal processing goes here.
			*output = filter.doTPTMoogLPF(*signal);
			// Increment buffer pointers.
			++signal;
			++pitch;
			++resonance;
			++output;
		}
	}

	virtual void onSetPins() override;
	virtual int32_t MP_STDCALL open() override;

private:
	AudioInPin pinSignal;
	AudioInPin pinPitch;
	AudioInPin pinResonance;
	FloatInPin pinSaturation;
	IntInPin pinMode;
	BoolInPin pinSaturator;
	AudioOutPin pinOutput;
};

// Using original code untouched for reference.
class LadderFilter_test : public MpBase2
{
	CMoogLadderFilter filter;

public:
	LadderFilter_test();
	void subProcess(int sampleFrames);
	virtual void onSetPins() override;
	virtual int32_t MP_STDCALL open() override;

private:
	AudioInPin pinSignal;
	AudioInPin pinPitch;
	AudioInPin pinResonance;
	FloatInPin pinSaturation;
	IntInPin pinMode;
	BoolInPin pinSaturator;
	AudioOutPin pinOutput;
};
#endif

