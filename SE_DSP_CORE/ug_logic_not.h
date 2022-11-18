// ug_logic_not module
//
#pragma once
#include "ug_generic_1_1.h"

class ug_logic_not : public ug_generic_1_1
{
public:
	ug_logic_not();
	DECLARE_UG_BUILD_FUNC(ug_logic_not);
	void sub_process(int start_pos, int sampleframes) override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	void OnFirstSample();
	int Open() override;

private:
	void input_changed();
	bool cur_state;
	float output;
	state_type in_state;
};
