#pragma once

#include "ug_control.h"
#include "smart_output.h"

class ug_slider : public ug_control
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_slider);

	ug_slider();
	int Open() override;
	void Resume() override;
	void sub_process(int start_pos, int sampleframes);
	void ChangeOutput(bool p_smoothing);
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state ) override;
private:
	float patchValue_;
	short appearance;

	smart_output output_so;
};
