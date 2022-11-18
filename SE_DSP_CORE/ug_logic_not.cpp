#include "pch.h"
#include "ug_logic_not.h"
#include "Logic.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_logic_not)

namespace
{
REGISTER_MODULE_1_BC(56,L"NOT Gate", IDS_MN_NOT_GATE,IDS_MG_LOGIC,ug_logic_not ,CF_STRUCTURE_VIEW,L"Simulated Logic Gate. Output is opposit of input.");
}

// invert input, unless indeterminate voltage
void ug_logic_not::sub_process(int start_pos, int sampleframes)
{
	float* in = in1_ptr + start_pos;
	float* out = out1_ptr + start_pos;
	/*
		for( int s = sampleframes ; s > 0 ; s-- )
		{
			*out++ = 0.1f * *in1++ / *in2++;
		}
		return;
	*/
	int count = sampleframes;
	float* last = in + sampleframes;
	timestamp_t end_clock = SampleClock() + sampleframes;

	//	_RPT0(_CRT_WARN, "--------------\n" );
	while( count > 0 )
	{
		//		_RPT1(_CRT_WARN, "count %d\n", count );
		// duration till next in change?
		if( cur_state )
		{
			while( *in >= LOGIC_LO && in < last )
			{
				in++;
				*out++ = output;
			}
		}
		else
		{
			while( *in <= LOGIC_HI && in < last )
			{
				in++;
				*out++ = output;
			}
		}

		count = (int)(last - in);

		if( count > 0 )
		{
			SetSampleClock(end_clock - count);
			input_changed();

			// must be here, after each posible change in output level.
			// not at end of sub_process!
			if( in_state < ST_RUN )
			{
				SleepIfOutputStatic(count);
			}
		}
	}
}

void ug_logic_not::input_changed()
{
	if( check_logic_input( GetPlug(0)->getValue(), cur_state ) )
	{
		if( cur_state )
		{
			output = 0.0f;
		}
		else
		{
			output = 0.5f;
		}

		SET_CUR_FUNC( &ug_logic_not::sub_process );
		ResetStaticOutput();
		OutputChange( SampleClock(), GetPlug(PN_OUT1), ST_ONE_OFF );
	}
}

void ug_logic_not::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	//	Wake();
	// any input running?
	in_state = p_state;

	if( in_state == ST_RUN )
	{
		SET_CUR_FUNC( &ug_logic_not::sub_process );
		ResetStaticOutput();
	}
	else
	{
		input_changed();
	}
}

ug_logic_not::ug_logic_not() :
	output(0.5f)
	,cur_state(false)
	,in_state(ST_STOP)
{
}

int ug_logic_not::Open()
{
	RUN_AT( SampleClock(), &ug_logic_not::OnFirstSample );
	return ug_generic_1_1::Open();
}

void ug_logic_not::OnFirstSample()
{
	OutputChange( SampleClock(), GetPlug(PN_OUT1), ST_ONE_OFF );
}