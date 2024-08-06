// ug_filter_biquad module
//
#pragma once

#include "ug_base.h"

class ULookup;

class ug_filter_biquad : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_filter_biquad);
	ug_filter_biquad();
	int Open() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

	void process_all(int start_pos, int sampleframes);
	void process_fixed_freq(int start_pos, int sampleframes);
	void process_no_input(int start_pos, int sampleframes);
	//	void process_static(int start_pos, int sampleframes);

	// This provides a counter to check the filter periodically to see it it has 'crashed'.
	inline void doStabilityCheck()
	{
		if (--stabilityCheckCounter < 0)
		{
			StabilityCheck();
			stabilityCheckCounter = 50;
		}
	}
	void StabilityCheck();
	void freq_change( float p_pitch, float p_resonance );

	void freq_change_1pole();

	// Inputs
	short freq_scale;

private:
	// moog vfc vars. In order of access
	float k,/* xn,*/ xnm1, pp1d2, kp, y1n, y1nm1, y2n, y2nm1, y3n, y3nm1, y4n;
	float scale;
	float overdrive;

	float max_stable_freq;
	float quality;
	ULookup* lookup_table;
	ULookup* lookup_table2;

	// parametric eq vars
	float yn, inv_a0, a1, a2, b0, b1, b2;
	float xnm2,ynm2,ynm1;

	float* input_ptr;
	float* pitch_ptr;
	float* resonance_ptr;
	float* output_ptr;

	bool output_quiet;
	int stabilityCheckCounter = 0;
};

