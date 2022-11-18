// ug_filter_1pole module
//
#pragma once

#include "ug_base.h"
class ULookup;

class ug_filter_1pole : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_filter_1pole);

	ug_filter_1pole();
	int Open() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

	void process_all_lp(int start_pos, int sampleframes);
	void process_fixed_freq_lp(int start_pos, int sampleframes);
	void process_static(int start_pos, int sampleframes);
	void process_all_hp(int start_pos, int sampleframes);
	void process_fixed_freq_hp(int start_pos, int sampleframes);
	void process_fixed_freq_lp_settling(int start_pos, int sampleframes);
	void process_fixed_freq_hp_settling(int start_pos, int sampleframes);

	inline void freq_change( float p_pitch );
	void OnFirstSample();

	// This provides a counter to check the filter periodically to see it it has 'crashed'.
	inline void doStabilityCheck()
	{
		if (--stabilityCheckCounter < 0)
		{
			StabilityCheck();
			stabilityCheckCounter = 50;
		}
	}

	// override this to check if your filter memory contains invalid data.
	void StabilityCheck();

private:
	float y1n;
	float l;
	float k;

	ULookup* lookup_table;

	float* input_ptr;
	float* pitch_ptr;
	float* output_ptr;

	bool output_quiet;

	short freq_scale;
protected:
	bool low_pass_mode;

	int stabilityCheckCounter = 0;
};

class ug_filter_1pole_hp : public ug_filter_1pole
{
public:
	DECLARE_UG_BUILD_FUNC(ug_filter_1pole_hp);
	ug_filter_1pole_hp();
};
