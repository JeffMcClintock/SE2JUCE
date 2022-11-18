#ifndef SVFILTER_H_INCLUDED
#define SVFILTER_H_INCLUDED

#include "../shared/xp_simd.h"
#include "../shared/FilterBase.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>

class StateVariableFilterBase : public FilterBase
{
protected:
	class FilterPitchChanging
	{
	public:
		const static int pitchTableLowVolts = -4; // ~ 1Hz
		const static int pitchTableHiVolts = 11;  // ~ 20kHz.

		// Fast version using table.
		inline static float ComputeIncrement2(const float* pitchTable, float pitch)
		{
			// TODO: see OscillatorNaive.h for faster method.
			const float maxPitchValue = pitchTableHiVolts * 0.1f;
			const float minPitchValue = pitchTableLowVolts * 0.1f;
			if(!(pitch < maxPitchValue)) // reverse test to catch also Nans.
			{
				pitch = maxPitchValue;
			}
			else
			{
				if(pitch < minPitchValue)
					pitch = minPitchValue;
			}

			pitch *= 120.0f;

			int table_floor = FastRealToIntTruncateTowardZero(pitch); // fast float-to-int using SSE. truncation toward zero.
			float fraction = pitch - (float)table_floor;

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

		inline static void CalcInitial(const float* pitchTable, float pitch, float returnIncrement)
		{
			// do nothing. Hopefully optimizes away to nothing.
		}
		inline static void Calculate(const float* pitchTable, float pitch, float& returnIncrement)
		{
			returnIncrement = ComputeIncrement2(pitchTable, pitch);
		}
		inline static void IncrementPointer(float*& pitch)
		{
			++pitch;
		}
		enum { Active = true };
	};

	class FilterPitchFixed
	{
	public:
		inline static void CalcInitial(const float* pitchTable, float pitch, float& returnIncrement)
		{
			return FilterPitchChanging::Calculate(pitchTable, pitch, returnIncrement);
		}
		inline static void Calculate(const float* pitchTable, float pitch, float returnIncrement)
		{
			// do nothing. Hopefully optimizes away to nothing.
		}
		inline static void IncrementPointer(const float* pitch)
		{
			// do nothing. Hopefully optimizes away to nothing.
		}
		enum { Active = false };
	};

	class ResonanceChanging
	{
	public:
		inline static void CalcInitial(float resonance, float pitch, float returnQuality)
		{
			// do nothing. Hopefully optimizes away to nothing.
		}
		inline static void Calculate(float resonance, float pitch, float& returnQuality)
		{
			returnQuality = (std::max)(1.0f - resonance, 0.001f);
		}
		inline static void IncrementPointer(float*& pointer)
		{
			++pointer;
		}
		enum { Active = true };
	};

	class ResonanceFixed
	{
	public:
		inline static void CalcInitial(float resonance, float pitch, float& returnQuality)
		{
			return ResonanceChanging::Calculate(resonance, pitch, returnQuality);
		}
		inline static void Calculate(float resonance, float pitch, float returnQuality)
		{
			// do nothing. Hopefully optimizes away to nothing.
		}
		inline static void IncrementPointer(const float* pitch)
		{
			// do nothing. Hopefully optimizes away to nothing.
		}
		enum { Active = false };
	};

	class FilterModeLowPass
	{
	public:
		inline static float SelectOutput(float input, float R, float& lowPass, float& highPass, float& bandPass)
		{
			return lowPass;
		}
	};

	class FilterModeHighPass
	{
	public:
		inline static float SelectOutput(float input, float R, float& lowPass, float& highPass, float& bandPass)
		{
			return highPass;
		}
	};

	class FilterModeBandPass
	{
	public:
		inline static float SelectOutput(float input, float R, float& lowPass, float& highPass, float& bandPass)
		{
			return bandPass;
		}
	};

	class FilterModeBandReject // Notch.
	{
	public:
		inline static float SelectOutput(float input, float R, float& lowPass, float& highPass, float& bandPass)
		{
			return input - 2.0f * R * bandPass;
		}
	};

	class FilterModeBandShelving
	{
	public:
		inline static float SelectOutput(float input, float R, float& lowPass, float& highPass, float& bandPass)
		{
			return 0.0f; // this (don't know what m_fK is sposed to be) input + 2.0f *R * m_fK * bandPass;
		}
	};

	class FilterModePeaking
	{
	public:
		inline static float SelectOutput(float input, float R, float& lowPass, float& highPass, float& bandPass)
		{
			return input - 4.0f * R * bandPass;
		}
	};

protected:
	AudioInPin pinSignal;
	AudioInPin pinPitch;
	AudioInPin pinResonance;
	AudioOutPin pinOutput;

	// TPZ vars.
	// Stage 1.
	float m_fZA1;
	float m_fZB1;
	// Stage 2.
	float m_fZA2;
	float m_fZB2;

	float *pitchTable;
	float *stabilityTable;
	float coef_g;
	float coef_R;

public:
	StateVariableFilterBase() :
		m_fZA1(0.0f)
		, m_fZB1(0.0f)
		, m_fZA2(0.0f)
		, m_fZB2(0.0f)
	{}

	int32_t MP_STDCALL open() override;
	virtual void onSetPins() override;
	virtual bool isFilterSettling() override
	{
		return !pinSignal.isStreaming() && !pinPitch.isStreaming() && !pinResonance.isStreaming();
	}
	virtual void StabilityCheck() override;
	virtual AudioOutPin& getOutputPin() override
	{
		return pinOutput;
	}

	virtual void ChoseProcessMethod() = 0;
};

class StateVariableFilter3 : public StateVariableFilterBase
{
public:
	StateVariableFilter3();

	template< class PitchModulationPolicy, class ResonanceModulationPolicy, class FilterModePolicy, int stages >
	void subProcessZdf(int sampleFrames)
	{
		doStabilityCheck();

		float* signal = getBuffer(pinSignal);
		float* pitch = getBuffer(pinPitch);
		float* resonance = getBuffer(pinResonance);
		float* output = getBuffer(pinOutput);

		for (int s = sampleFrames; s > 0; --s)
		{
			ResonanceModulationPolicy::Calculate(*resonance, *pitch, coef_R);
			PitchModulationPolicy::Calculate(pitchTable, *pitch, coef_g);

			/* old style
			// stage 1.
			lowPass += bandPass * f1;
			float highPass = *signal - bandPass * quality - lowPass;
			bandPass += highPass * f1;
			*/

			// TPT-style zero-delay feedback.
			float xn = *signal;
			float highPass = (xn - (2.0f * coef_R + coef_g) * m_fZA1 - m_fZB1) / (1.0f + 2.0f * coef_R * coef_g + coef_g * coef_g);
			float bandPass = coef_g * highPass + m_fZA1;
			float lowPass = coef_g * bandPass + m_fZB1;

			// z1 register update
			m_fZA1 = coef_g * highPass + bandPass;
			m_fZB1 = coef_g * bandPass + lowPass;

			*output = FilterModePolicy::SelectOutput(xn, coef_R, lowPass, highPass, bandPass);

			if (stages == 2)
			{
				xn = *output;
				highPass = (xn - (2.0f * coef_R + coef_g) * m_fZA2 - m_fZB2) / (1.0f + 2.0f * coef_R * coef_g + coef_g * coef_g);
				bandPass = coef_g * highPass + m_fZA2;
				lowPass = coef_g * bandPass + m_fZB2;

				// z1 register update
				m_fZA2 = coef_g * highPass + bandPass;
				m_fZB2 = coef_g * bandPass + lowPass;

				*output = FilterModePolicy::SelectOutput(xn, coef_R, lowPass, highPass, bandPass);
			}

			// Increment buffer pointers.
			++signal;
			++output;
			PitchModulationPolicy::IncrementPointer(pitch);
			ResonanceModulationPolicy::IncrementPointer(resonance);
		}
	}

	virtual void ChoseProcessMethod() override;
	void OnFilterSettled() override;

protected:
	IntInPin pinMode;
	IntInPin pinStrength;
};

class StateVariableFilter3B : public StateVariableFilterBase
{
	AudioOutPin pinHiPass;
	AudioOutPin pinBandPass;
	AudioOutPin pinBandReject;

public:
	StateVariableFilter3B();

	template< class PitchModulationPolicy, class ResonanceModulationPolicy >
	void subProcessZdf(int sampleFrames)
	{
		doStabilityCheck();

		float* signal = getBuffer(pinSignal);
		float* pitch = getBuffer(pinPitch);
		float* resonance = getBuffer(pinResonance);
		float* lowpass = getBuffer(pinOutput);
		float* hipass = getBuffer(pinHiPass);
		float* bandpass = getBuffer(pinBandPass);
		float* bandreject = getBuffer(pinBandReject);

		for (int s = sampleFrames; s > 0; --s)
		{
			// calc freq factor
			ResonanceModulationPolicy::Calculate(*resonance, *pitch, coef_R);
			PitchModulationPolicy::Calculate(pitchTable, *pitch, coef_g);

			// TPT-style zero-delay feedback.
			float xn = *signal;
			float highPass = (xn - (2.0f * coef_R + coef_g) * m_fZA1 - m_fZB1) / (1.0f + 2.0f * coef_R * coef_g + coef_g * coef_g);
			float bandPass = coef_g * highPass + m_fZA1;
			float lowPass = coef_g * bandPass + m_fZB1;

			// z1 register update
			m_fZA1 = coef_g * highPass + bandPass;
			m_fZB1 = coef_g * bandPass + lowPass;

			*lowpass = lowPass; //  FilterModeLowPass::SelectOutput(xn, coef_R, lowPass, highPass, bandPass);
			*hipass = highPass; // FilterModeHighPass::SelectOutput(xn, coef_R, lowPass, highPass, bandPass);
			*bandpass = bandPass; // FilterModeBandPass::SelectOutput(xn, coef_R, lowPass, highPass, bandPass);
			*bandreject = FilterModeBandReject::SelectOutput(xn, coef_R, lowPass, highPass, bandPass);

			// Increment buffer pointers.
			++signal;
			++lowpass;
			++hipass;
			++bandpass;
			++bandreject;
			PitchModulationPolicy::IncrementPointer(pitch);
			ResonanceModulationPolicy::IncrementPointer(resonance);
		}
	}

	void subProcessStatic(int sampleFrames)
	{
		float* lowpass = getBuffer(pinOutput);
		float* hipass = getBuffer(pinHiPass);
		float* bandpass = getBuffer(pinBandPass);
		float* bandreject = getBuffer(pinBandReject);

		for( int s = sampleFrames; s > 0; s-- )
		{
			*lowpass++ = static_output;
			*hipass++ = 0.0f;
			*bandpass++ = 0.0f;
			*bandreject++ = static_output;
		}
	}

	virtual void ChoseProcessMethod() override;
	virtual void OnFilterSettled() override;
};

#endif

