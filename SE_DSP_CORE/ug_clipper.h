#pragma once
#include "ug_base.h"

class ug_clipper : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_clipper);

	void sub_process(int start_pos, int sampleframes);
	void process_all_stop(int start_pos, int sampleframes);
	int Open() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

private:
	float* input_ptr;
	float* output_ptr;
	float* hi_lim_ptr;
	float* lo_lim_ptr;
};
