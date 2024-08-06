// ug_test_tone module
//
#pragma once

#include "ug_base.h"

class ug_test_tone : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_test_tone);

	ug_test_tone();
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	int Open() override;
	void sub_process(int start_pos, int sampleframes);
private:
	double m_phase;
	double m_increment;
	float* out_ptr;
};
