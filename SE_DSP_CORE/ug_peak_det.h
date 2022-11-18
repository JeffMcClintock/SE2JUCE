// ug_peak_det module
//
#pragma once

#include "ug_base.h"
#include "sample.h"

class ug_peak_det : public ug_base
{
public:
	int Open() override;
	void sub_process(int start_pos, int sampleframes);
	void sub_process_all_run(int start_pos, int sampleframes);
	void sub_process_settling(int start_pos, int sampleframes);
	void process_static(int start_pos, int sampleframes);
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_peak_det);

	ug_peak_det();
private:
	double current_level;
	float* input_ptr;
	float* attack_ptr;
	float* decay_ptr;
	float* output_ptr;
	double ga;
	double gr;
};
