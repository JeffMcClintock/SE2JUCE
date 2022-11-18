#pragma once
#include "ug_math_base.h"

class ug_multiplier : public ug_math_base
{
public:

	ug_multiplier();
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

	void process_both_run(int start_pos, int sampleframes) override;
	void process_A_run(int start_pos, int sampleframes) override;
	void process_B_run(int start_pos, int sampleframes) override;

	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_multiplier);
};

