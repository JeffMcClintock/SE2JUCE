#pragma once
#include "pluginconstants.h"

class CTPTMoogFilterStage
{
public:
	CTPTMoogFilterStage();
	~CTPTMoogFilterStage();

protected:
	// controls
	float G;			// cutoff
	float scalar;		// scalar value
	float sampleRate;   // fs
	float z1;			// z-1 storage location

public:
	inline void setSampleRate(float newSampleRate)
	{
		// save
		sampleRate = newSampleRate;
	}

	inline void initialize(float newSampleRate)
	{
		// save
		setSampleRate(newSampleRate);

		// flush delay
		z1 = 0;
	}

	void setFc(float fc);
	float doFilterStage(float xn);
	float getSampleRate(){return sampleRate;}
	float getStorageRegisterValue(){return z1;}
};
