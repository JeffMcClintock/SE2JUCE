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
	//void UpdateOutput(int patch, bool p_smoothing = false);
	//bool DoAutomation(float p_normalised_value);
	//std::wstring GetValue();
	//float GetAutomationValue();
private:
	float patchValue_;
	float output_val;
	float max_val;
	float min_val;
	short appearance;

	smart_output output_so;
};
