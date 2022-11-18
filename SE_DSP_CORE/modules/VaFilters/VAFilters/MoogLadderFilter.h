#pragma once
#include "MoogFilterStage.h"

class CMoogLadderFilter
{
public:
	CMoogLadderFilter();
	~CMoogLadderFilter();

protected:
	CMoogFilterStage filter1;
	CMoogFilterStage filter2;
	CMoogFilterStage filter3;
	CMoogFilterStage filter4;

	// avoid zero delay loop
	float zMinusOneRegister;

	float k; // Q control

public:
	inline void initialize(float newSampleRate)
	{
		filter1.initialize(newSampleRate);
		filter2.initialize(newSampleRate);
		filter3.initialize(newSampleRate);
		filter4.initialize(newSampleRate);

		// flush our delay
		zMinusOneRegister = 0.0;
	}
	inline void setSampleRate(float newSampleRate)
	{
		filter1.setSampleRate(newSampleRate);
		filter2.setSampleRate(newSampleRate);
		filter3.setSampleRate(newSampleRate);
		filter4.setSampleRate(newSampleRate);
	}
	// calculate coeffs using the Moog parameters; the knobs go from
	// 0 to 10 on a Moog Synth; this function accepts values from 
	// 0 to 10 for moog_fc and moog_Q
	inline void calculateCoeffsMoog(float moog_fc, float moog_Q)
	{
		// make sure 0 -> 10!
		moog_fc = min(moog_fc, 10.0);
		moog_fc = max(moog_fc, 0);
		moog_Q = min(moog_Q, 10.0);
		moog_Q = max(moog_Q, 0);

		filter1.calculateCoeffs(moog_fc);
		filter2.calculateCoeffs(moog_fc);
		filter3.calculateCoeffs(moog_fc);
		filter4.calculateCoeffs(moog_fc);

		// Q = 0->10 ==> k = 0->4
		moog_Q /= 10.0;
		k = 4.0*moog_Q;

		// bound it; will blow up at high fc
		k = min(k, 3.4);
	}

	// calculate the Coeffs using "normal" Fc and Q values
	// NOTE: this version suffers from a non linear relationship
	// with Fc and the pole calculation. See the Paper on the
	// website for more info
	inline void calculateCoeffs(float fc, float Q)
	{
		// fc = 0 -> Nyquist ==> moog_fc = -1.0 -> 0.16
		float moog_fc = fc/filter1.getSampleRate(); // 0 -> 0.5
		moog_fc *= 20.0; // 0 -> 10

		Q = max(Q, 0.707);
		float moog_Q = (Q - 0.707)/(25.0 - 0.707); // 0 -> 1
		moog_Q *= 10;	// 0 -> 10

		calculateCoeffsMoog(moog_fc, moog_Q);
	}

	float doMoogLPF(float xn);
};
