#include "pch.h"
#include "ug_multiplier.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_multiplier)

namespace
{
	REGISTER_MODULE_1(L"Ring Modulator", IDS_MN_RING_MODULATOR, IDS_MG_EFFECTS, ug_multiplier, CF_STRUCTURE_VIEW, L"One input controls the level of the other.  It can be used for ring modulation, or to apply an envelope to a signal ( amplitude modulation ), or to scale a signal by a fixed amount.");
	REGISTER_MODULE_1(L"Level Adj", IDS_MN_LEVEL_ADJ, IDS_MG_MODIFIERS, ug_multiplier, CF_STRUCTURE_VIEW, L"This multiplies one input by the other.  It can be used for ring modulation, or to apply an envelope to a signal ( amplitude modulation ), or to scale a signal by a fixed amount.  Note: to apply an volume envelope use the DCA module (it changes the volume of the signal on a decibel scale, not a linear scale).");
}

#define PN_INPUT1	0
#define PN_INPUT2	1
#define PN_OUT		2

// Fill an array of InterfaceObjects with plugs and parameters

IMPLEMENT_UG_INFO_FUNC2(ug_multiplier)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	LIST_PIN2( L"Input 1", in_ptr[0], DR_IN, L"8", L"", IO_LINEAR_INPUT, L"The 2 inputs are Multiplied. Then 'normalised' eg 5V multiplied by 2 V = 1V, (5 * 2 ) / 10 ");
	LIST_PIN2( L"Input 2", in_ptr[1], DR_IN, L"8", L"", IO_LINEAR_INPUT, L"");
	LIST_PIN2( L"Output",  out1_ptr,DR_OUT, L"0", L"", 0, L"");
}

void ug_multiplier::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	const state_type input1_status = GetPlug(PN_INPUT1)->getState();
	const state_type input2_status = GetPlug(PN_INPUT2)->getState();
    state_type out_stat = (std::max)(input1_status, input2_status);

	bypassPlug = -1;

	if (input1_status < ST_RUN)
	{
		if (input2_status == ST_RUN)
		{
			auto val1 = GetPlug(PN_INPUT1)->getValue();
			if (val1 == 0.0f)
			{
				out_stat = ST_ONE_OFF;
			}
			else
			{
				if (val1 == 1.0f)
				{
					bypassPlug = 1; // 2nd.
				}
			}
		}
	}
	else
	{
		if (input2_status < ST_RUN)
		{
			auto val2 = GetPlug(PN_INPUT2)->getValue();
			if (val2 == 0.0f)
			{
				out_stat = ST_ONE_OFF;
			}
			else
			{
				if (val2 == 1.0f)
				{
					bypassPlug = 0; // 1st.
				}
			}
		}
	}

	if (bypassPlug >= 0)
	{
		SET_CUR_FUNC(&ug_multiplier::process_bypass);
	}
	else
	{
		if (out_stat < ST_RUN)
		{
			SET_CUR_FUNC(&ug_math_base::process_both_stop1);
			ResetStaticOutput();
		}
		else
		{
			if (input1_status < ST_RUN)
			{
				SET_CUR_FUNC(&ug_math_base::process_B_run);
			}
			else
			{
				if (input2_status < ST_RUN)
				{
					SET_CUR_FUNC(&ug_math_base::process_A_run);
				}
				else
				{
					SET_CUR_FUNC(&ug_math_base::process_both_run);
				}
			}
		}
	}

	GetPlug(PN_OUT)->TransmitState(p_clock, out_stat);
}

void ug_multiplier::process_both_run(int start_pos, int sampleframes)
{
	const float* in1 = in_ptr[0] + start_pos;
	const float* in2 = in_ptr[1] + start_pos;
	float* __restrict out = out1_ptr + start_pos;

#if 0
	// No SSE. Use C++.
	for (int s = sampleframes; s > 0; --s)
	{
		*out++ = *in1++ * *in2++;
	}
#else
	// auto-vectorize.
	while (sampleframes > 3)
	{
		out[0] = in1[0] * in2[0];
		out[1] = in1[1] * in2[1];
		out[2] = in1[2] * in2[2];
		out[3] = in1[3] * in2[3];

		out += 4;
		in1 += 4;
		in2 += 4;
		sampleframes -= 4;
	}

	while (sampleframes > 0)
	{
		*out++ = *in1++ * *in2++;
		--sampleframes;
	}
#endif
}

void ug_multiplier::process_A_run(int start_pos, int sampleframes)
{
	const float* in1 = in_ptr[0] + start_pos;
	const float* in2 = in_ptr[1] + start_pos;
	float* __restrict out = out1_ptr + start_pos;
	const float gain = *in2;

#if 1
	// No SSE. Use C++.
	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = *in1++ * gain;
	}
#else
	// auto-vectorize (hopefully). no faster.
	while (sampleframes > 3)
	{
		out[0] = in1[0] * gain;
		out[1] = in1[1] * gain;
		out[2] = in1[2] * gain;
		out[3] = in1[3] * gain;

		out += 4;
		in1 += 4;
		in2 += 4;
		sampleframes -= 4;
	}

	while (sampleframes > 0)
	{
		*out++ = *in1++ * gain;
		--sampleframes;
	}
#endif
}

void ug_multiplier::process_B_run(int start_pos, int sampleframes)
{
	const float* in1 = in_ptr[0] + start_pos;
	const float* in2 = in_ptr[1] + start_pos;
	float* __restrict out = out1_ptr + start_pos;
	const float gain = *in1;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = *in2++ * gain;
	}
}

ug_multiplier::ug_multiplier()
{
	can_sleep_if_one_input_zero = true;
}

