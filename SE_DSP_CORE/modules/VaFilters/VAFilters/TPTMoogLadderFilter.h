#pragma once
#include "TPTMoogFilterStage.h"

class CTPTMoogLadderFilter
{
public:
	CTPTMoogLadderFilter();
	~CTPTMoogLadderFilter();

protected:
	CTPTMoogFilterStage filter1;
	CTPTMoogFilterStage filter2;
	CTPTMoogFilterStage filter3;
	CTPTMoogFilterStage filter4;

	float k; // Q control
	float fc; // fc control

public:
	inline void setSampleRate(float newSampleRate)
	{
		filter1.setSampleRate(newSampleRate);
		filter2.setSampleRate(newSampleRate);
		filter3.setSampleRate(newSampleRate);
		filter4.setSampleRate(newSampleRate);
	}

	inline void initialize(float newSampleRate)
	{
		filter1.initialize(newSampleRate);
		filter2.initialize(newSampleRate);
		filter3.initialize(newSampleRate);
		filter4.initialize(newSampleRate);
	}

	inline void calculateTPTCoeffs(float cutoff, float Q)
	{
		// account for bandwidth shrinkage? you could try this.
		// see "Active Network Design with Signal Filtering Applications" by C. Lindquist, p. 100
		// we know the combination of sync tuned filters will have a shrunken bandwidth
		// so the cutoff will not be at the proper location
		// usually not a problem for musical applications

		// 4 sync-tuned filters
		filter1.setFc(cutoff);
		filter2.setFc(cutoff);
		filter3.setFc(cutoff);
		filter4.setFc(cutoff);
				
		// NOTE: Q is limited to 18 to prevent blowing up
		// Q = 0.707 -> 25 ==> k = 0->4
		k = 4.0*(Q - 0.5)/(25.0 - 0.5);

		// ours
		fc = cutoff;
	}

	float doTPTMoogLPF(float xn);
};
