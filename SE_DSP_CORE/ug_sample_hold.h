#pragma once

#include "ug_base.h"

class ug_sample_hold : public ug_base
{
public:
	ug_sample_hold();
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_sample_hold);

	void sub_process(int start_pos, int sampleframes);
	void sub_process_static(int start_pos, int sampleframes);
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

private:
	float* input_ptr;
	float* hold_ptr;
	float* output_ptr;
	bool gate_state;
	float cur_output;
};

