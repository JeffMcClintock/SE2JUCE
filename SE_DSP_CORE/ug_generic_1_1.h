// ug_generic_1_1 module
//
#pragma once

#include "ug_base.h"

#define PN_IN1 0
#define PN_OUT1 1

class ug_generic_1_1 : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;

	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	int Open() override;
	virtual void sub_process(int start_pos, int sampleframes) = 0;
	void sub_process_static(int start_pos, int sampleframes);

protected:
	float* in1_ptr;
	float* out1_ptr;
};

// Simple example of using ug_generic_1_1
class ug_rectifier : public ug_generic_1_1
{
public:

    DECLARE_UG_BUILD_FUNC(ug_rectifier);

    void sub_process(int start_pos, int sampleframes) override;
};
