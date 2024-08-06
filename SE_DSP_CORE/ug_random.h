// ug_random module
//
#pragma once
#include "ug_base.h"
#include "smart_output.h"

class ug_random : public ug_base
{
public:
	ug_random();
	DECLARE_UG_BUILD_FUNC(ug_random);
	DECLARE_UG_INFO_FUNC2;
	void sub_process(int start_pos, int sampleframes);
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	int Open() override;
	void Resume() override;
private:
	void ChangeOutput();
	bool m_trigger;
	smart_output output_so;
};
