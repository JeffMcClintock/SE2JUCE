// ug_system_command module
//
#pragma once
#include "ug_generic_1_1.h"

class ug_system_command : public ug_generic_1_1
{
public:
	ug_system_command();
	DECLARE_UG_BUILD_FUNC(ug_system_command);
	DECLARE_UG_INFO_FUNC2;
	void sub_process(int start_pos, int sampleframes) override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
private:
	void input_changed();
	bool cur_state;
	state_type in_state;
	std::wstring m_command;
};
