#pragma once
#include "ug_base.h"
#include "modules/shared/xp_simd.h"

#define PN_INPUT1	0
#define PN_INPUT2	1
#define PN_OUT		2

class ug_math_base : public ug_base
{
public:
	ug_math_base();
	virtual void process_both_run(int start_pos, int sampleframes) = 0;
	virtual void process_A_run(int start_pos, int sampleframes) = 0;
	virtual void process_B_run(int start_pos, int sampleframes) = 0;
	void process_both_stop1(int start_pos, int sampleframes);
	void process_both_stop2(int start_pos, int sampleframes);
	virtual void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

	void process_bypass(int start_pos, int sampleframes)
	{
		const float* in = in_ptr[bypassPlug] + start_pos;
		float* __restrict out = out1_ptr + start_pos;

		// auto-vectorized copy.
		while (sampleframes > 3)
		{
			out[0] = in[0];
			out[1] = in[1];
			out[2] = in[2];
			out[3] = in[3];

			out += 4;
			in += 4;
			sampleframes -= 4;
		}

		while (sampleframes > 0)
		{
			*out++ = *in++;
			--sampleframes;
		}
	}

	bool PPGetActiveFlag() override;

protected:
	float* in_ptr[2];
	float* out1_ptr;
	bool can_sleep_if_one_input_zero;
	float static_output_val;
	int bypassPlug = -1;
};

class ug_math_div : public ug_math_base

{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_math_div);

	void process_both_run(int start_pos, int sampleframes) override;
	void process_A_run(int start_pos, int sampleframes) override;
	void process_B_run(int start_pos, int sampleframes) override;
};

class ug_math_mult : public ug_math_base
{
public:
	ug_math_mult();

	void process_both_run(int start_pos, int sampleframes) override;
	void process_A_run(int start_pos, int sampleframes) override;
	void process_B_run(int start_pos, int sampleframes) override;

	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_math_mult);
};

class ug_math_sub : public ug_math_base
{
public:
	void process_both_run(int start_pos, int sampleframes) override;
	void process_A_run(int start_pos, int sampleframes) override;
	void process_B_run(int start_pos, int sampleframes) override;

	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_math_sub);
};
