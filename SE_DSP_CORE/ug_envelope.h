#pragma once
#include "sample.h"

#include "ug_envelope_base.h"

// 8 stage envelope
#define ENV_STEPS 8

class ug_envelope : public ug_envelope_base
{
	friend class ug_envelope_base; // for testing
public:
	ug_envelope();
	inline float CalcIncrement(float p_rate);
	void ChooseSubProcess() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	void sub_process5(int start_pos, int sampleframes);
	void sub_process8(int start_pos, int sampleframes);
	void sub_process9(int start_pos, int sampleframes);

protected:
	float fixed_levels[ENV_STEPS];
	UPlug* rate_plugs[ENV_STEPS];
	UPlug* level_plugs[ENV_STEPS];
};
