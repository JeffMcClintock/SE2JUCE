#ifndef KORGFILTER_H_INCLUDED
#define KORGFILTER_H_INCLUDED

#include "../shared/xp_simd.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include "../shared/FilterBase.h"
#include "VAOnePoleFilter.h"
 
class KorgFilter : public FilterBase
{
	class PitchChanging
	{
	public:
		inline static void CalcInitial(const float* pitchTable, float pitch, float returnIncrement)
		{
			// do nothing. Hopefully optimizes away to nothing.
		}
		inline static void Calculate(const float* pitchTable, float pitch, float& return_g)
		{
		}
		inline static void IncrementPointer(float*& pitch)
		{
			++pitch;
		}
		enum { Active = true };
	};

	class PitchFixed
	{
	public:
		inline static void CalcInitial(const float* pitchTable, float pitch, float& returnIncrement)
		{
			return PitchChanging::Calculate(pitchTable, pitch, returnIncrement);
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
			// Roughly match old SVF Resonance range.
			float res = (std::min)(resonance, pitch + 0.2f); // Restrict max resonance at lower pitch.
			// testin notch		res = (std::max)(res, -1.0f);
			returnQuality = 0.4f - 0.7f * res; // nice scale.

			//		returnQuality = (std::max)(0.005f, 1.0f - resonance);
			returnQuality = (std::max)(0.001f, 1.0f - resonance);
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

	const static int pitchTableLowVolts = -4; // ~ 1Hz
	const static int pitchTableHiVolts = 11;  // ~ 20kHz.

	enum{ LPF1, HPF1 }; /* one short string for each */
	double m_dFc;
	double m_dSaturation;
	bool m_uNonLinearProcessing;
	double alpha0;   // our u scalar value
	float m_dK;
	float safeMaxHz = {};

	enum FilterMode { LowPass, HiPass };
protected:
	CVAOnePoleFilter m_LPF1;
	CVAOnePoleFilter m_LPF2;	// Lo-Pass only;
	CVAOnePoleFilter m_HPF1;
	CVAOnePoleFilter m_HPF3;	// Hi-Pass only;

public:
	KorgFilter();

	inline float calcSafeResonance(float resonance) const
	{
		// Upper reso limit applies only when saturation is off
		const float maxStableResonance = pinSaturation.getValue() ? 5.0f : 1.0f;
		return (std::max)(0.0f, (std::min)(maxStableResonance, resonance));
	}

	inline void UpdateFiltersLP( float pitch, float resonance )
	{
		const auto safeResonance = calcSafeResonance(resonance);

		float hz = 440.f * powf(2.f, (10.0f * pitch) - 5.f);
		hz = (std::min)(hz, safeMaxHz);

		m_dK = 0.01f + safeResonance * (2.0f - 0.01f); // ? K (0.01 - 2.0)

		// use this is f you want to let filters update themselves;
		// since we calc everything here, it would be redundant

		// sample
		// prewarp for BZT
		float G;
		double wd = 2 * M_PI * hz; //  m_dFc;
		double T = 1 / (double)getSampleRate();
		double wa = (2 / T)*tan(wd*T / 2);
		double g = wa*T / 2;

		// G - the feedforward coeff in the VA One Pole
		G = static_cast<float>(g / (1.0 + g));

// G should be in lookup table.

		// set alphas
		m_LPF1.m_fAlpha = G;
		m_LPF2.m_fAlpha = G;
		m_HPF1.m_fAlpha = G;

		// set betas all are in the form of  <something>/((1 + g)
		m_LPF2.m_fBeta = (m_dK - m_dK*G) / (1.0f + static_cast<float>(g) );
		m_HPF1.m_fBeta = static_cast<float>(-1.0 / (1.0 + g));

		// set alpha0 variable
		alpha0 = 1.0 / (1.0 - m_dK*G + m_dK*G*G);
	}

	inline void UpdateFiltersHP(float pitch, float resonance)
	{
		const auto safeResonance = calcSafeResonance(resonance);

		float hz = 440.f * powf(2.f, (10.0f * pitch) - 5.f);
		hz = (std::min)(hz, safeMaxHz);
		m_dK = 0.01f + safeResonance * (2.0f - 0.01f); // ? K (0.01 - 2.0)

		// prewarp for BZT
		double wd = 2 * M_PI * m_dFc;
		double T = 1 / (double)getSampleRate();
		double wa = (2 / T)*tan(wd*T / 2);
		double g = wa*T / 2;

		// G - the feedforward coeff in the VA One Pole
		const float G = static_cast<float>(g / (1.0 + g));

		// set alphas
		m_HPF1.m_fAlpha = G;
		m_LPF1.m_fAlpha = G;
		m_HPF3.m_fAlpha = G;

		// set betas all are in the form of  <something>/((1 + g)(1 - kG + kG^2))
		m_HPF3.m_fBeta = static_cast<float>(-1.0*G / (1.0 + g));
		m_LPF1.m_fBeta = static_cast<float>(1.0 / (1.0 + g));

		// set th G35H variable
		alpha0 = 1.0 / (1.0 - m_dK*G + m_dK*G*G);

		return;
	}

	template< class PitchModulationPolicy, int HiPassMode >
	void subProcessZdf(int sampleFrames)
	{
		doStabilityCheck();

		// get pointers to in/output buffers.
		float* signal = getBuffer(pinSignal);
		float* pitch = getBuffer(pinPitch);
		float* resonance = getBuffer(pinResonance);
		float* output = getBuffer(pinOutput);

		for (int s = sampleFrames; s > 0; --s)
		{
			// calc freq factor
//			ResonanceModulationPolicy::Calculate(*resonance, *pitch, coef_R);
//			PitchModulationPolicy::Calculate(pitchTable, *pitch, coef_g);
			if (PitchModulationPolicy::Active) // or resonance.
			{
				UpdateFiltersLP(*pitch, *resonance);
			}

			float xn = *signal;

			// process input through LPF1
			double y1 = m_LPF1.doFilterLp(xn);

			// form feedback value
			double S35 = m_HPF1.getFeedbackOutput() + m_LPF2.getFeedbackOutput();

			// calculate u
			double u = alpha0*(y1 + S35);

			// NAIVE NLP
			if (m_uNonLinearProcessing )
			{
				// Regular Version
				u = tanh(m_dSaturation*u);
			}

			// feed it to LPF2
			auto y = m_dK * m_LPF2.doFilterLp(static_cast<float>(u));

			// feed y to HPF
			m_HPF1.doFilterHp(y);

			// auto-normalize
			if (m_dK > 0)
				y *= 1 / m_dK;

			*output = y; //  FilterModePolicy::SelectOutput(xn, coef_R, lowPass, highPass, bandPass);

//			PushHistory(*output);

			// Increment buffer pointers.
			++signal;
			++output;
			PitchModulationPolicy::IncrementPointer(pitch);
			PitchModulationPolicy::IncrementPointer(resonance); // pitch and resonance intimatly related.
		}
	}

	int32_t MP_STDCALL open() override;
	void onSetPins() override;

	// Support for FilterBase
	virtual bool isFilterSettling() override
	{
		return !pinSignal.isStreaming() && !pinPitch.isStreaming() && !pinResonance.isStreaming();
	}

	void OnFilterSettled() override
	{
		if (pinMode != 0) // LP
			static_output = 0.0f;

		FilterBase::OnFilterSettled();
	}

	virtual AudioOutPin& getOutputPin() override
	{
		return pinOutput;
	}

	void StabilityCheck() override
	{
		if (!m_LPF1.isStable())
		{
			m_LPF1.reset();
		}
		if (!m_LPF2.isStable())
		{
			m_LPF2.reset();
		}
		if (!m_HPF1.isStable())
		{
			m_HPF1.reset();
		}
		if (!m_HPF3.isStable())
		{
			m_HPF3.reset();
		}
	}

protected:
	AudioInPin pinSignal;
	AudioInPin pinPitch;
	AudioInPin pinResonance;
	AudioOutPin pinOutput;
	IntInPin pinMode;
	FloatInPin pinSaturation;
	BoolInPin pinSaturationEnable;

	float* pitchTable;
	float* stabilityTable;
	float coef_g;
	float coef_R;
};

#endif


