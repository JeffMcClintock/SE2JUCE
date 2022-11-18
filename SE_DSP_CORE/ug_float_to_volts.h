// ug_float_to_volts module
//
#pragma once

#include "ug_base.h"
#include "smart_output.h"

class ug_float_to_volts : public ug_base
{
public:
	DECLARE_UG_BUILD_FUNC(ug_float_to_volts);
	void assignPlugVariable(int p_plug_desc_id, UPlug* p_plug) override;
	int Open() override;
	void Resume() override;
	void sub_process(int start_pos, int sampleframes);
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
private:
	float m_input;
	smart_output value_out_so;
	short m_smoothing;
};

