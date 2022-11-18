// ug_logic_complex module
// base class for logic device with many inputs, many outputs

#pragma once
#include "ug_base.h"
#include "smart_output.h"
#include <vector>

class ug_logic_complex : public ug_base
{
public:
	ug_logic_complex();
	~ug_logic_complex();
	int Open() override;
	virtual void input_changed() {}

	void process_run(int start_pos, int sampleframes);
	void sub_process(int start_pos, int sampleframes);
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

protected:
	smart_output* outputs; // array of smart outputs

	std::vector<bool> in_level; // list of plug logic levels
	float** in_ptr; // ARRAY OF TEMP POINTERS to input samples
	bool inputs_running;
	int numInputs;
	int numOutputs;
};
