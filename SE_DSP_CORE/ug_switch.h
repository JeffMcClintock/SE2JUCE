#pragma once

#include "ug_base.h"

// #define USE_BYPASS

class ug_switch : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_switch);

	ug_switch();
	~ug_switch();
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
#ifdef USE_BYPASS
	virtual void DoProcess(int buffer_offset, int sampleframes) override;
#endif
	void sub_process(int start_pos, int sampleframes);
	void sub_process2(int start_pos, int sampleframes);
	void sub_process_static(int start_pos, int sampleframes);
	int Open() override;
	void OnNewSetting( void);
	void Resume() override;

private:
	int* static_output_counts;
	UPlug* current_output_plug;
	float* in_ptr;
	float* out_ptr;
	state_type cur_state;
	short out_num;
	int current_output_plug_number;
	int numOutputs;
};

class ug_switch2 : public ug_base
{
public:
	void OnOutStatChanged(state_type new_state);
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_switch2);

	ug_switch2();
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
#ifdef USE_BYPASS
	virtual void DoProcess(int buffer_offset, int sampleframes) override;
#endif
	void sub_process(int start_pos, int sampleframes);
	void sub_process_static(int start_pos, int sampleframes);
	void sub_process_no_inputs(int start_pos, int sampleframes);
	int Open() override;
	void OnNewSetting();

private:
	UPlug* in_plg;
	float* out_ptr;
	std::vector<state_type> in_state; // list of plug states
	short safe_in_num; // constrained to legal values
	short unchecked_in_num;
	int numInputs;
};