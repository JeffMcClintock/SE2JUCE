#include "MoogLadderFilter.h"

CMoogLadderFilter::CMoogLadderFilter()
{
	zMinusOneRegister = 0.0;
}

CMoogLadderFilter::~CMoogLadderFilter()
{
}

float CMoogLadderFilter::doMoogLPF(float xn)
{
	// get feedback value
	float fb = zMinusOneRegister*k;

	float input = xn - fb;

	// four in series
	float filterOut = filter4.doFilterStage(filter3.doFilterStage(filter2.doFilterStage(filter1.doFilterStage(input))));

	// save for loop
	zMinusOneRegister = filterOut;

	return filterOut;
}
