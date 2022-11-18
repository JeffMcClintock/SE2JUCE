#ifndef SVFILTER2_H_INCLUDED
#define SVFILTER2_H_INCLUDED

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include "../../shared/FilterBase.h"
#include "../../shared/xp_simd.h"

#undef min
#undef max

const static int pitchTableLowVolts = -4; // ~ 1Hz
const static int pitchTableHiVolts = 11;  // ~ 20kHz.

// Fast version using table.
inline float ComputeIncrement2( const float* pitchTable, float pitch )
{
	const float maxPitchValue = pitchTableHiVolts * 0.1f;
	const float minPitchValue = pitchTableLowVolts * 0.1f;
	if (!(pitch < maxPitchValue)) // reverse test to catch also Nans.
	{
		pitch = maxPitchValue;
	}
	else
	{
		if (pitch < minPitchValue)
			pitch = minPitchValue;
	}

	pitch *= 120.0f;

	int table_floor = FastRealToIntTruncateTowardZero(pitch); // fast float-to-int using SSE. truncation toward zero.
	float fraction = pitch - (float) table_floor;

	// Linear interpolator.
	const float* p = pitchTable + table_floor;
	return p[0] + fraction * (p[1] - p[0]);

	/*
	// Cubic interpolator. Too slow. Can't hear such precision in a filter anyhow.
	float y0 = pitchTable[table_floor-1];
	float y1 = pitchTable[table_floor+0];
	float y2 = pitchTable[table_floor+1];
	float y3 = pitchTable[table_floor+2];

	return y1 + 0.5f * fraction*(y2 - y0 + fraction * (2.0f*y0 - 5.0f*y1 + 4.0f*y2 - y3 + fraction*(3.0f*(y1 - y2) + y3 - y0)));
	*/
}

class PitchFixed
{
public:
	inline static void CalcInitial( const float* pitchTable, float pitch, float MaxStableF1, float& returnIncrement )
	{
		returnIncrement = ComputeIncrement2( pitchTable, pitch );
		returnIncrement = std::min( returnIncrement, MaxStableF1 );
	}
	inline static void Calculate( const float* pitchTable, float pitch, float MaxStableF1, float& returnIncrement )
	{
		// do nothing. Hopefully optimizes away to nothing.
	}
	inline static void IncrementPointer( const float* pitch )
	{
		// do nothing. Hopefully optimizes away to nothing.
	}
	enum { Active = false };
};

class PitchChanging
{
public:
	inline static void CalcInitial( const float* pitchTable, float pitch, float MaxStableF1, float& returnIncrement )
	{
		// do nothing. Hopefully optimizes away to nothing.
	}
	inline static void Calculate( const float* pitchTable, float pitch, float MaxStableF1, float& returnIncrement )
	{
		returnIncrement = ComputeIncrement2( pitchTable, pitch );
		returnIncrement = std::min( returnIncrement, MaxStableF1 );
	}
	inline static void IncrementPointer( float*& pitch )
	{
		++pitch;
	}
	enum { Active = true };
};

class ResonanceChanging
{
public:
	inline static void CalcInitial( const float* stabilityTable, float resonance, float& returnMaxStableF1, float& returnQuality )
	{
		// do nothing. Hopefully optimizes away to nothing.
	}
	inline static void Calculate( const float* stabilityTable, float resonance, float& returnMaxStableF1, float& returnQuality )
	{
		// limit q to resonable values (under 9.992V)
		float q = resonance;
		q = std::max( q, 0.f );
		q = std::min( q, 0.9992f );
		returnQuality = 2.f - 2.f * q; // map to 0 - 2.0 range

		const int tableFloor = FastRealToIntTruncateTowardZero(q * 512.f); // fast float-to-int using SSE. truncation toward zero.
		returnMaxStableF1 = stabilityTable[ tableFloor ];
	}
	inline static void IncrementPointer( float*& pitch )
	{
		++pitch;
	}
	enum { Active = true };
};

class ResonanceFixed
{
public:
	inline static void CalcInitial( const float* stabilityTable, float resonance, float& returnMaxStableF1, float& returnQuality )
	{
		return ResonanceChanging::Calculate(stabilityTable, resonance, returnMaxStableF1, returnQuality);
	}
	inline static void Calculate( const float* stabilityTable, float resonance, float& returnMaxStableF1, float& returnQuality )
	{
		// do nothing. Hopefully optimizes away to nothing.
	}
	inline static void IncrementPointer( const float* pitch )
	{
		// do nothing. Hopefully optimizes away to nothing.
	}
	enum { Active = false };
};

class FilterModeLowPass
{
public:
	inline static float SelectOutput( float& lowPass, float& highPass, float& bandPass )
	{
		return lowPass;
	}
};

class FilterModeHighPass
{
public:
	inline static float SelectOutput( float& lowPass, float& highPass, float& bandPass )
	{
		return highPass;
	}
};

class FilterModeBandPass
{
public:
	inline static float SelectOutput( float& lowPass, float& highPass, float& bandPass )
	{
		return bandPass;
	}
};

class FilterModeBandReject
{
public:
	inline static float SelectOutput(float& lowPass, float& highPass, float& bandPass)
	{
		return highPass + lowPass;
	}
};

/* TODO
 An allpass filter may also be
built with the four amplifier configuration by subtracting the
band-pass output from the input. In this instance, the band-pass
gain must equal 2.

class FilterModeAllPass
{
public:
	inline static float SelectOutput(float& lowPass, float& highPass, float& bandPass)
	{
		return highPass + lowPass;
	}
};
*/

class SvFilter2 : public FilterBase
{
public:
	SvFilter2();
	virtual void initializePins();

	template< class PitchModulationPolicy, class ResonanceModulationPolicy, class FilterModePolicy, int stages >
	void subProcess( int sampleFrames )
	{
		doStabilityCheck();

		auto signal = getBuffer(pinSignal);
		auto pitch = getBuffer(pinPitch);
		auto resonance = getBuffer(pinResonance);
		auto output = getBuffer(pinOutput);

		float l_lp1 = low_pass1;
		float l_bp1 = band_pass1;

		float l_lp2,l_bp2;
		if( stages == 2 )
		{
			l_lp2 = low_pass2;
			l_bp2 = band_pass2;
		}

		for( int s = sampleFrames; s > 0; --s )
		{
			// calc freq factor
			ResonanceModulationPolicy::Calculate( stabilityTable, *resonance, maxStableF1, quality );
			PitchModulationPolicy::Calculate( pitchTable, *pitch, maxStableF1, f1 );

			// stage 1.
			l_lp1 += l_bp1 * f1;
			float hi_pass = *signal - l_bp1 * quality - l_lp1;
			l_bp1 += hi_pass * f1;

			*output = FilterModePolicy::SelectOutput( l_lp1, hi_pass, l_bp1 );

			if( stages == 2 )
			{
				// stage 2.
				l_lp2 += l_bp2 * f1;
				hi_pass = *output - l_bp2 * quality - l_lp2;
				l_bp2 += hi_pass * f1;

				*output = FilterModePolicy::SelectOutput( l_lp2, hi_pass, l_bp2 );
			}

			// Increment buffer pointers.
			++signal;
			++output;
			PitchModulationPolicy::IncrementPointer( pitch );
			ResonanceModulationPolicy::IncrementPointer( resonance );
		}

		// store delays
		low_pass1 = l_lp1;
		band_pass1 = l_bp1;
		if( stages == 2 )
		{
			low_pass2 = l_lp2;
			band_pass2 = l_bp2;
		}
	}

	virtual int32_t MP_STDCALL open() override;
	virtual void onSetPins() override;
	virtual void ChoseProcessMethod();

	void StabilityCheck() override;

	virtual bool isFilterSettling() override
	{
		return !pinSignal.isStreaming() && !pinPitch.isStreaming() && !pinResonance.isStreaming();
	}
	virtual AudioOutPin& getOutputPin() override
	{
		return pinOutput;
	}
	void OnFilterSettled() override
	{
		if (pinMode != 0 && pinMode != 3) // LP, BR
		{
			static_output = 0.0f;
		}
		FilterBase::OnFilterSettled();
	}

protected:
	AudioInPin pinSignal;
	AudioInPin pinPitch;
	AudioInPin pinResonance;
	AudioOutPin pinOutput;
	IntInPin pinMode;
	IntInPin pinStrength;

	float *pitchTable;
	float *stabilityTable;
	float low_pass1;
	float band_pass1;
	float low_pass2;
	float band_pass2;
	float maxStableF1;
	float quality;
	float f1;
};

#endif

