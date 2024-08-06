#pragma once

#include "ug_base.h"

class ug_voltage_to_enum : public ug_base
{
public:
	virtual void CalcLimits( int p_idx );
	ug_voltage_to_enum();
	~ug_voltage_to_enum();
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_voltage_to_enum);
	virtual void Setup( class ISeAudioMaster* am, class TiXmlElement* xml ) override;
	ug_base* Clone( CUGLookupList& UGLookupList ) override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	int Open() override;
	void sub_process(int start_pos, int sampleframes);

protected:
	void OnChange(timestamp_t p_clock, float p_input);
	float* in_ptr;
	float m_low;
	float m_hi;
	short enum_out;
	float m_out_val_count;
	short* m_enum_vals;
	std::wstring m_enum_list;
};

class ug_voltage_to_enum2 : public ug_voltage_to_enum
{
public:
	DECLARE_UG_BUILD_FUNC(ug_voltage_to_enum2);
	void CalcLimits(int p_idx) override;
};

class ug_int_to_enum : public ug_base
{
public:
	ug_int_to_enum();
	~ug_int_to_enum();
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_int_to_enum);
	virtual void Setup(class ISeAudioMaster* am, class TiXmlElement* xml) override;
	ug_base* Clone( CUGLookupList& UGLookupList ) override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	int Open() override;

private:
	void OnChange(timestamp_t p_clock, int32_t p_input);
	int32_t m_in;
	short enum_out;
	int32_t m_out_val_count;
	short* m_enum_vals;
	std::wstring m_enum_list;
	bool setupWasCalled = false;
};