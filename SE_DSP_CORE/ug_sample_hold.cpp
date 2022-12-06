#include "pch.h"
#include "ug_sample_hold.h"

#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_sample_hold)



namespace
{
REGISTER_MODULE_1(L"Sample and Hold", IDS_MN_SAMPLE_AND_HOLD,IDS_MG_MODIFIERS,ug_sample_hold ,CF_STRUCTURE_VIEW,L"Sample and Hold Module");
}

#define PLG_SIGNAL		0
#define PLG_HOLD		1
#define PLG_OUTPUT		2

// Fill an array of InterfaceObjects with plugs and parameters
void ug_sample_hold::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	LIST_PIN2( L"Audio", input_ptr, DR_IN, L"8", L"", IO_LINEAR_INPUT, L"");
	LIST_PIN2( L"Hold", hold_ptr, DR_IN, L"8", L"", IO_POLYPHONIC_ACTIVE, L"Freezes input signal on leading edge");
	LIST_PIN2( L"Output", output_ptr, DR_OUT, L"0", L"", 0, L"");
}

ug_sample_hold::ug_sample_hold() :
	gate_state(true) //false) fix to allow trigger on startup with fixed Hold = high
	,cur_output(0.f)
{
}

void ug_sample_hold::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	if( p_to_plug == GetPlug(PLG_HOLD) )
	{
		OutputChange( p_clock, GetPlug(PLG_OUTPUT), p_state );
	}

	if( GetPlug(PLG_SIGNAL)->getState() < ST_RUN && GetPlug(PLG_HOLD)->getState() < ST_RUN )
	{
		ResetStaticOutput();
		SET_CUR_FUNC( &ug_sample_hold::sub_process_static );
	}
	else
	{
		SET_CUR_FUNC( &ug_sample_hold::sub_process );
	}
}

void ug_sample_hold::sub_process_static(int start_pos, int sampleframes)
{
	sub_process(start_pos, sampleframes);
	SleepIfOutputStatic(sampleframes);
}

void ug_sample_hold::sub_process(int start_pos, int sampleframes)
{
	float* input = input_ptr + start_pos;
	float* hold = hold_ptr + start_pos;
	float* output = output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		bool new_gate = *hold++ > 0.0f;
		if( new_gate != gate_state )
		{
			gate_state = new_gate;
			if( gate_state )
			{
				cur_output = *input;
			}
		}

		*output++ = cur_output;
		input++;
	}
}

