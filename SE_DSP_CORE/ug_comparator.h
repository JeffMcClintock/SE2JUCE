#pragma once
#include "ug_base.h"

class ug_comparator : public ug_base
{
public:
	void process_both_run(int start_pos, int sampleframes);
	void process_A_run(int start_pos, int sampleframes);
	void process_B_run(int start_pos, int sampleframes);
	void process_both_stop(int start_pos, int sampleframes);
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_comparator);

	//	bool PPGetActiveFlag();
private:
	float* in1_ptr;
	float* in2_ptr;
	float* out1_ptr;
	float m_hi;
	float m_lo;
};
