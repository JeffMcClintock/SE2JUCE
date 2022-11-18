#include "TPTMoogLadderFilter.h"

CTPTMoogLadderFilter::CTPTMoogLadderFilter()
{
//	zMinusOneRegister = 0.0;
}

CTPTMoogLadderFilter::~CTPTMoogLadderFilter()
{
}

float CTPTMoogLadderFilter::doTPTMoogLPF(float xn)
{
	// calculate g
	float wd = 2*pi*fc;          
	float T  = 1/(float)filter1.getSampleRate();             
	float wa = (2/T)*tan(wd*T/2); 
	float g  = wa*T/2;  

	float G = g/(1.0 + g);

	float GAMMA = G*G*G*G;
	float S1 = filter1.getStorageRegisterValue()/(1.0 + g);
	float S2 = filter2.getStorageRegisterValue()/(1.0 + g);
	float S3 = filter3.getStorageRegisterValue()/(1.0 + g);
	float S4 = filter4.getStorageRegisterValue()/(1.0 + g);

	float SIGMA = G*G*G*S1 + G*G*S2 + G*S3 + S4;

	// u is input to filters
	float u = (xn - k*SIGMA)/(1 + k*GAMMA);

	// four in series
	float filterOut = filter4.doFilterStage(filter3.doFilterStage(filter2.doFilterStage(filter1.doFilterStage(u))));

	return filterOut;
}
