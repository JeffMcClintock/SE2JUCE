#pragma once
#include "rateconvertor.h"

class CDecimator : public CRateConvertor
{
public:
	CDecimator(void);
	~CDecimator(void);

	virtual bool decimateNextOutputSample(float xnL, float xnR, float& fLeftOutput, float& fRightOutput);
	virtual void decimateSamples(float* pLeftDeciBuffer, float* pRightDeciBuffer,float& ynL, float& ynR);
};

