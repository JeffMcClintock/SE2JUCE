#ifndef RMS_H_INCLUDED
#define RMS_H_INCLUDED

#include "../SvFilterClassic/SvFilter2.h"
#include "../shared/xp_simd.h"

class Rms : public SvFilter2
{
	const float pitchAdjustment = 0.5f;
/*
	static const int tableEntries = 256;
	static const int extraEntriesAtStart = 1; // for interpolator.
	static const int extraEntriesAtEnd = 3; // for interpolator.
*/

public:
	virtual void initializePins() override;
	virtual void onSetPins() override;
	virtual void ChoseProcessMethod() override;

	inline float fastSquareRoot(float v)
	{
#if !GMPI_USE_SSE
        return sqrtf(v); // MCC++ 0.0265 CPU. (but with SSE enabled).. (needs max(0,v) )
#else
		// 0.0265 CPU. Winner.
		float r;
		const __m128 staticZero = _mm_setzero_ps();
		_mm_store_ss(&r, _mm_sqrt_ss(_mm_max_ps(_mm_load_ss(&v), staticZero)));
		return r;
#endif


#if 0
		//0.039 CPU.
		float index = v * (float)(tableEntries / 2); // table covers 20V, hence division by 2.
		int table_floor = FastRealToIntTruncateTowardZero(index);

		if (table_floor <= 0) // indicated index *might* be less than zero. e.g. Could be 0.1 which is valid, or -0.1 which is not.
		{
			if (!(index >= 0.0f)) // reverse logic to catch Nans.
			{
				return 0.0f;
			}
		}
		else
		{
			if (table_floor >= tableEntries)
			{
				// Approximate sqrt greater than 2.0 with straight line.
				const float root2 = 1.4142135623730950488016887242097f;
				const float slope = 0.31f;
				return (v - 2.0f) * slope + root2;

//				return sqrtTable[tableEntries];
			}
		}

		float fraction = index - (float)table_floor;

#if 0
		// Due to discontinutity in sqrt at zero, use linear interpolation on first table entry.
		if (table_floor < 1)
		{
			// return sqrtTable[0] + fraction * (sqrtTable[1] - sqrtTable[0]);
			return fraction * sqrtTable[1]; // sqrtTable[0] is zero and can be eliminated.
		}
#endif

		// Cubic interpolator.
		assert(table_floor >= 0 && table_floor <= tableEntries);

		float y0 = sqrtTable[table_floor - 1];
		float y1 = sqrtTable[table_floor + 0];
		float y2 = sqrtTable[table_floor + 1];
		float y3 = sqrtTable[table_floor + 2];

		return y1 + 0.5f * fraction * (y2 - y0 + fraction * (2.0f*y0 - 5.0f*y1 + 4.0f*y2 - y3 + fraction*(3.0f*(y1 - y2) + y3 - y0)));
#endif

	}

	template< class PitchModulationPolicy, class ResonanceModulationPolicy, class FilterModePolicy, int stages >
	void subProcess(int sampleFrames)
	{
		if (--stabilityCheckCounter < 0)
		{
			StabilityCheck();
		}

		auto signal = getBuffer(pinSignal);
		auto pitch = getBuffer(pinPitch);
		auto output = getBuffer(pinOutput);

		float l_lp1 = low_pass1;
		float l_bp1 = band_pass1;

		float l_lp2, l_bp2;
		if (stages == 2)
		{
			l_lp2 = low_pass2;
			l_bp2 = band_pass2;
		}

		for (int s = sampleFrames; s > 0; --s)
		{
			// calc freq factor
			PitchModulationPolicy::Calculate(pitchTable, *pitch * pitchAdjustment, maxStableF1, f1);

			auto squared = *signal * *signal;

			// stage 1.
			l_lp1 += l_bp1 * f1;
			float hi_pass = squared - l_bp1 * quality - l_lp1;
			l_bp1 += hi_pass * f1;

			float mean = FilterModePolicy::SelectOutput(l_lp1, hi_pass, l_bp1);

			if (stages == 2)
			{
				// stage 2.
				l_lp2 += l_bp2 * f1;
				hi_pass = mean - l_bp2 * quality - l_lp2;
				l_bp2 += hi_pass * f1;

				mean = FilterModePolicy::SelectOutput(l_lp2, hi_pass, l_bp2);
			}

			*output = fastSquareRoot(mean);

			// Increment buffer pointers.
			++signal;
			++output;
			PitchModulationPolicy::IncrementPointer(pitch);
		}

		// store delays
		low_pass1 = l_lp1;
		band_pass1 = l_bp1;
		if (stages == 2)
		{
			low_pass2 = l_lp2;
			band_pass2 = l_bp2;
		}
	}

	virtual void OnFilterSettled() override
	{
		FilterBase::OnFilterSettled();
	}

private:
	float* sqrtTable;
};

#endif

