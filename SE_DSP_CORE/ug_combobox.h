#pragma once
#include "ug_control.h"

class ug_combobox : public ug_control
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_combobox);

	ug_combobox();
	int Open() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state ) override;
	//	std::wstring GetValue();
	//	bool DoAutomation(float p_normalised_value);
	//	float GetAutomationValue();
	//	std::wstring m_enum_list;
	//	void UpdateOutput(int patch, bool p_smoothing = false);
private:
	short patchValue_;
	short EnumOut;
};
