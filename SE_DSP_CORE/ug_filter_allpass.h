// ug_filter_allpass module
//
#pragma once
#include "ug_base.h"

class ULookup;

class ug_filter_allpass : public ug_base
{
public:
	void OverflowCheck();
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_filter_allpass);

	ug_filter_allpass() : output_quiet(true) {}

	int Open() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

	void process_all(int start_pos, int sampleframes);
	void process_fixed_freq(int start_pos, int sampleframes);
	void process_static(int start_pos, int sampleframes);

	float delay1;
	float delay2;
	// rearranged for efficient access
	float factor_A;
	float factor_B;
	short freq_scale;
private:
	inline void freq_change( float p_pitch, float p_resonance );

	ULookup* lookup_tableA;
	ULookup* lookup_tableB;

	float* input_ptr;
	float* pitch_ptr;
	float* resonance_ptr;
	float* output_ptr;

	bool output_quiet;
};
