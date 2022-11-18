// ug_filter_sv module
//
#pragma once
#include "ug_base.h"
#include "sample.h"

class ug_filter_sv : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_filter_sv);
	int Open() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	void SetOutputState( timestamp_t p_clock, state_type p_state );
	float CalcFreqFactor(float p_pitch );
	float CalcQ(float p_resonance );
	void process_static(int start_pos, int sampleframes);
	void process_all(int start_pos, int sampleframes);
	void process_fixed_lp(int start_pos, int sampleframes);
	void process_fixed_hp(int start_pos, int sampleframes);
	void process_both_run(int start_pos, int sampleframes);

	void OverflowCheck();
	ug_filter_sv();

	float band_pass;
	float low_pass;

	// Inputs
	ULookup* lookup_table;
	ULookup* max_freq_lookup;

	float max_stable_freq;
	short freq_scale;

private:
	void OnFirstSample();
	float CalcMSF(float p_resonance );
	bool only_lp_used;
	bool only_hp_used;
	float* m_lookup_table_data;
	float factor1;
	float debug_freq_hz;
	float fixed_quality_factor; // precalc resonance if not changing

	float* input_ptr;
	float* pitch_ptr;
	float* resonance_ptr;
	float* lp_ptr;
	float* hp_ptr;
	float* bp_ptr;
	float* br_ptr;
	bool output_quiet;
};
