#pragma once
#include <assert.h>
#include <math.h> 

class CVAOnePoleFilter
{
public:
	CVAOnePoleFilter();
	
	// common variables
	float m_fSampleRate;	/* sample rate*/
	float m_fFc;			/* cutoff frequency */
	
	int m_uFilterType;		/* filter selection */
	enum{LPF1,HPF1}; /* one short string for each */

	// Trapezoidal Integrator Components
	float m_fAlpha;			// Feed Forward coeff
	float m_fBeta;			// Feed Back coeff

	// provide access to our feedback output
	float getFeedbackOutput(){return m_fZ1*m_fBeta;}

	// -- CFilter Overrides --
	void reset(){m_fZ1 = 0;}

	// recalc the coeff -- NOTE: not used for Korg35 Filter
	void updateFilter();
	
	// do the filter
	inline float doFilterLp(float xn)
	{
		// calculate v(n)
		float vn = (xn - m_fZ1)*m_fAlpha;

		// form LP output
		float lpf = vn + m_fZ1;

		// update memory
		m_fZ1 = vn + lpf;

		assert(m_uFilterType != HPF1);
		return lpf;
	}

	inline float doFilterHp(float xn)
	{
		// calculate v(n)
		float vn = (xn - m_fZ1)*m_fAlpha;

		// form LP output
		float lpf = vn + m_fZ1;

		// update memory
		m_fZ1 = vn + lpf;

		// do the HPF
		float hpf = xn - lpf;

		assert(m_uFilterType == HPF1);
		return hpf;
	}

	bool isStable()
	{
		return !isnan(m_fZ1) && isfinite(m_fZ1);
	}
protected:
	float m_fZ1;		// our z-1 storage location
};

