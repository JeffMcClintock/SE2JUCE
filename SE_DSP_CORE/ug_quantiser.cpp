#include "pch.h"
#include <algorithm>
#include "ug_quantiser.h"
#include <math.h>
#include "module_register.h"

using namespace std;



SE_DECLARE_INIT_STATIC_FILE(ug_quantiser)

namespace
{
REGISTER_MODULE_1(L"Quantiser", IDS_MN_QUANTISER,IDS_MG_MODIFIERS,ug_quantiser,CF_STRUCTURE_VIEW,L"Use to contrain the voltage to discrete 'steps', e.g. to the nearest whole Volt.");
}

// Fill an array of InterfaceObjects with plugs and parameters
void ug_quantiser::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, CLIPPER, Default, defid (index into unit_gen::PlugFormats)
	// defid used to CLIPPER a enum list or range of values
	LIST_PIN2( L"Signal In", in_ptr[0],  DR_IN, L"0", L""		, IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN2( L"Step Size", in_ptr[1],  DR_IN, L"1", L""	, IO_POLYPHONIC_ACTIVE, L"Quantisation step size in Volts");
	LIST_PIN2( L"Signal Out",out1_ptr,  DR_OUT, L"", L"", 0, L"");
}

void ug_quantiser::onSetPin( timestamp_t p_clock, UPlug* p_to_plug, state_type p_state )
{
	state_type input1_status = GetPlug( PN_INPUT1 )->getState( );
	state_type input2_status = GetPlug( PN_INPUT2 )->getState( );

	if( input1_status == ST_RUN && input2_status < ST_RUN && GetPlug(PN_INPUT2)->getValue() <= 0.0f)
	{
		SET_CUR_FUNC( &ug_quantiser::processBypass );
		GetPlug(PN_OUT)->TransmitState(p_clock, input1_status);
	}
	else
	{
		if( input1_status < ST_RUN && GetPlug(PN_INPUT1)->getValue() == 0.0f) // Input zero and stopped. Will still process, but by outputing ST_ONE_OFF it will allow voice to sleep.
		{
			SET_CUR_FUNC(&ug_math_base::process_both_stop1);
			ResetStaticOutput();
			GetPlug(PN_OUT)->TransmitState(p_clock, ST_ONE_OFF);
		}
		else
		{
			ug_math_base::onSetPin(p_clock, p_to_plug, p_state);
		}
	}
}

void ug_quantiser::processBypass( int start_pos, int sampleframes )
{
	float* input = in_ptr[0] + start_pos;
	float* output = out1_ptr + start_pos;

	for( int s = sampleframes; s > 0; s-- )
	{
		*output++ = *input++;
	}
}

void ug_quantiser::process_both_run(int start_pos, int sampleframes)
{
	float* input = in_ptr[0] + start_pos;
	float* output = out1_ptr + start_pos;
	float* step_size = in_ptr[1] + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float ss = max(*step_size, 0.000000001f ); // avoid divide by zero
		float nearest = floorf((*input++ + ss * 0.5f) / ss) * ss;
		*output++ = nearest;
		step_size++;
	}
}

void ug_quantiser::process_A_run(int start_pos, int sampleframes)
{
	float* input = in_ptr[0] + start_pos;
	float* output = out1_ptr + start_pos;
	float* step_size = in_ptr[1] + start_pos;
	float ss = max(*step_size, 0.000000001f ); // avoid divide by zero

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float nearest = floorf((*input++ + ss * 0.5f) / ss) * ss;
		*output++ = nearest;
	}
}

void ug_quantiser::process_B_run(int start_pos, int sampleframes)
{
	float* input = in_ptr[0] + start_pos;
	float* output = out1_ptr + start_pos;
	float* step_size = in_ptr[1] + start_pos;
	float in = *input;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float ss = max(*step_size, 0.000000001f ); // avoid divide by zero
		float nearest = floorf((in + ss * 0.5f) / ss) * ss;
		*output++ = nearest;
		step_size++;
	}
}