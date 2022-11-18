#include "TPTMoogFilterStage.h"

CTPTMoogFilterStage::CTPTMoogFilterStage()
{
	sampleRate = 44100;
}

CTPTMoogFilterStage::~CTPTMoogFilterStage()
{
}

void CTPTMoogFilterStage::setFc(float fc)
{
	// prewarp the cutoff- these are bilinear-transform filters
	float wd = 2*pi*fc;          
	float T  = 1/sampleRate;             
	float wa = (2/T)*tan(wd*T/2); 
	float g  = wa*T/2;            

	// calculate big G value; see Zavalishin p46 The Art of VA Design
	G = g/(1.0 + g);
}

// TPT 1 pole filter; see Zavalishin p46 The Art of VA Design
// note the zero-delay loop compensation has been removed from this code
// it is now a part of the overall cascade of filters (sum-of-s's)
float CTPTMoogFilterStage::doFilterStage(float xn)
{
	float v = (xn - z1)*G;
	float out = v + z1;
	z1 = out + v;

	return out;
}
