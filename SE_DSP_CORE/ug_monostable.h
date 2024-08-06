// ug_monostable module
//
#pragma once
#include "ug_base.h"

class ug_monostable : public ug_base
{
public:
	int sub_process2( float* in, float* out, int count);
	void pulse_end();
	ug_monostable();
	int Open() override;
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_monostable);
	void sub_process(int start_pos, int sampleframes);
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	void OnFirstSample();

private:
	void input_changed();

	float* in1_ptr;
	float* out1_ptr;
	timestamp_t pulse_off_time;
	bool cur_state;
	float output;
	state_type in_state;
};
