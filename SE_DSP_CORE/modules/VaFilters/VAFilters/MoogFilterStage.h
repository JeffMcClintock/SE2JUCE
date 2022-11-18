#pragma once
#include "pluginconstants.h"

class CMoogFilterStage :
	public CBiQuad
{
public:
	CMoogFilterStage();
	~CMoogFilterStage();

protected:
	// controls
	float rho; // cutoff
	float scalar;   // scalar value
	float sampleRate;   // scalar value

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
		flushDelays();
	}
	void calculateCoeffs(float fc);
	float doFilterStage(float xn);
	float getSampleRate(){return sampleRate;}
};
