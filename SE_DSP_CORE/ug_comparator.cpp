#include "pch.h"
#include <algorithm>
#include "ug_comparator.h"

#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_comparator)

using namespace std;

namespace
{
REGISTER_MODULE_1(L"Comparator", IDS_MN_COMPARATOR,IDS_MG_MODIFIERS,ug_comparator ,CF_STRUCTURE_VIEW,L"Compares the two input levels. If input A is greater, output is high (5), else low (-5).");
}

#define PN_INPUT1	0
#define PN_INPUT2	1
#define PN_OUT		2

// Fill an array of InterfaceObjects with plugs and parameters
IMPLEMENT_UG_INFO_FUNC2(ug_comparator)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	LIST_PIN2( L"Input A", in1_ptr, DR_IN, L"8", L"", IO_POLYPHONIC_ACTIVE, L"The 2 inputs are compared.  The output goes either high or low depending");
	LIST_PIN2( L"Input B", in2_ptr, DR_IN, L"8", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN2( L"Output",  out1_ptr,DR_OUT, L"0", L"", 0, L"");
	LIST_VAR3( L"Hi Out Val",m_hi, DR_IN, DT_FLOAT , L"5", L"",0, L"");
	LIST_VAR3( L"Lo Out Val",m_lo, DR_IN, DT_FLOAT , L"-5", L"",0, L"");
}

void ug_comparator::process_both_run(int start_pos, int sampleframes)
{
	float* in1 = in1_ptr + start_pos;
	float* in2 = in2_ptr + start_pos;
	float* out = out1_ptr + start_pos;
	float l_hi = m_hi * 0.1f;
	float l_lo = m_lo * 0.1f;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		if( *in1++ > *in2++ )
			*out++ = l_hi;
		else
			*out++ = l_lo;
	}
}

void ug_comparator::process_A_run(int start_pos, int sampleframes)
{
	float* in1 = in1_ptr + start_pos;
	float* in2 = in2_ptr + start_pos;
	float* out = out1_ptr + start_pos;
	float l_hi = m_hi * 0.1f;
	float l_lo = m_lo * 0.1f;
	float l_in2 = *in2;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		if( *in1++ > l_in2 )
			*out++ = l_hi;
		else
			*out++ = l_lo;
	}
}

void ug_comparator::process_B_run(int start_pos, int sampleframes)
{
	float* in1 = in1_ptr + start_pos;
	float* in2 = in2_ptr + start_pos;
	float* out = out1_ptr + start_pos;
	float l_hi = m_hi * 0.1f;
	float l_lo = m_lo * 0.1f;
	float l_in1 = *in1;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		if( l_in1 > *in2++ )
			*out++ = l_hi;
		else
			*out++ = l_lo;
	}
}

void ug_comparator::process_both_stop(int start_pos, int sampleframes)
{
	float* in1 = in1_ptr + start_pos;
	float* in2 = in2_ptr + start_pos;
	float* out = out1_ptr + start_pos;
	float out_val;

	if( *in1++ > *in2++ )
		out_val = m_hi * 0.1f;
	else
		out_val = m_lo * 0.1f;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = out_val;
	}

	SleepIfOutputStatic(sampleframes);
}
/*
bool ug_comparator::PPGetActiveFlag()
{
	// Multiplier is special.  Only active if BOTH inputs are active
	return GetPlug(0)->GetPPDownstreamFlag() && GetPlug(1)->GetPPDownstreamFlag();  // Multiplier is only polyphonic if both inputs are downstream
}
*/
void ug_comparator::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	state_type input1_status = GetPlug( PN_INPUT1 )->getState();
	state_type input2_status = GetPlug( PN_INPUT2 )->getState();
	state_type out_stat = max( input1_status, input2_status );

	if( out_stat < ST_RUN )
	{
		SET_CUR_FUNC( &ug_comparator::process_both_stop );
		ResetStaticOutput();
	}
	else
	{
		if( input1_status < ST_RUN )
		{
			SET_CUR_FUNC( &ug_comparator::process_B_run );
		}
		else
		{
			if( input2_status < ST_RUN )
			{
				SET_CUR_FUNC( &ug_comparator::process_A_run );
			}
			else
			{
				SET_CUR_FUNC( &ug_comparator::process_both_run );
			}
		}
	}

	// Avoid sending ST_STOP when m_hi or m_low changes, because successive ST-STOPs are ignored. We really mean ST_ONE_OFF.
	GetPlug(PN_OUT)->TransmitState( p_clock, out_stat == ST_RUN ? ST_RUN : ST_ONE_OFF );
}

