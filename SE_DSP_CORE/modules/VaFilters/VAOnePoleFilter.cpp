#include "VAOnePoleFilter.h"
#define _USE_MATH_DEFINES
#include <math.h>

CVAOnePoleFilter::CVAOnePoleFilter()
{
	m_fAlpha = 1.0;
	m_fBeta = 1.0;

	m_uFilterType = LPF1;
	reset();
}

// recalc coeffs -- NOTE: not used for Korg35 Filter
void CVAOnePoleFilter::updateFilter()
{
	float wd = 2 * (float)M_PI * m_fFc;          
	float T  = 1/m_fSampleRate;             
	float wa = (2/T)*tanf(wd*T/2); 
	float g  = wa*T/2;            

	m_fAlpha = g/(1.0f + g);
}


