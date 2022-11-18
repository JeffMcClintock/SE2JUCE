// This base class provides functionality of ug with
// a variable number of inputs
#pragma once
#include "ug_base.h"

class ug_logic_gate : public ug_base
{
public:
	ug_logic_gate();
	~ug_logic_gate();
	DECLARE_UG_INFO_FUNC2;
	int Open() override;
	void OnFirstSample();
	void RecalcOutput( timestamp_t p_sample_clock );
	void sub_process(int start_pos, int sampleframes);
	virtual bool OnInputLogicChange() = 0;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	void UpdateLevels();
//	void Setup DynamicPlugs();

protected:
	int input_count;
	std::vector<bool> in_level; // list of plug logic levels

private:
	bool output_changed;
	state_type overall_input_status;
	std::vector<state_type> in_state; // list of plug states
	float** in_ptr; // ARRAY OF TEMP POINTERS to input samples
	float output;
	float* output_blk;
};

// DERIVED CLASSES
class ug_logic_AND : public ug_logic_gate
{
public:
	bool OnInputLogicChange() override;
	DECLARE_UG_BUILD_FUNC(ug_logic_AND);
};

class ug_logic_NAND : public ug_logic_gate
{
public:
	bool OnInputLogicChange() override;
	DECLARE_UG_BUILD_FUNC(ug_logic_NAND);
};

class ug_logic_NOR : public ug_logic_gate
{
public:
	bool OnInputLogicChange() override;
	DECLARE_UG_BUILD_FUNC(ug_logic_NOR);
};

class ug_logic_OR : public ug_logic_gate
{
public:
	bool OnInputLogicChange() override;
	DECLARE_UG_BUILD_FUNC(ug_logic_OR);
};

class ug_logic_XOR : public ug_logic_gate
{
public:
	bool OnInputLogicChange() override;
	DECLARE_UG_BUILD_FUNC(ug_logic_XOR);
};

