#include "pch.h"
#include "ug_clipper.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_clipper)

using namespace std;

namespace
{
REGISTER_MODULE_1_BC(19,L"Clipper", IDS_MN_CLIPPER,IDS_MG_EFFECTS,ug_clipper ,CF_STRUCTURE_VIEW,L"Restricts (clips) the signal to a range between two Voltages.  Use this to distort the sound, or to limit a control signal between two values.");
}

#define PN_IN 0
#define PN_HI 1
#define PN_LO 2
#define PN_OUT 3

// Fill an array of InterfaceObjects with plugs and parameters
void ug_clipper::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, CLIPPER, Default, defid (index into unit_gen::PlugFormats)
	// defid used to CLIPPER a enum list or range of values
	LIST_PIN2( L"Signal In",input_ptr,  DR_IN, L"0", L""		, IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN2( L"Hi Limit",hi_lim_ptr,  DR_IN, L"5", L""	, IO_POLYPHONIC_ACTIVE, L"Clips any input signal above this voltage");
	LIST_PIN2( L"Lo Limit",lo_lim_ptr,  DR_IN, L"-2", L"-100, 0, 0, -10", IO_POLYPHONIC_ACTIVE, L"Clips any input signal below this voltage");
	LIST_PIN2( L"Signal Out",output_ptr,  DR_OUT, L"", L"", 0, L"");
}

int ug_clipper::Open()
{
	ug_base::Open();
	SET_CUR_FUNC( &ug_clipper::sub_process );
	return 0;
}

void ug_clipper::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	state_type input1_status = GetPlug( PN_IN )->getState();
	state_type input2_status = GetPlug( PN_LO )->getState();
	state_type input3_status = GetPlug( PN_HI )->getState();
	state_type out_stat = max( input1_status, input2_status );
	out_stat = max( out_stat, input3_status );

	if( out_stat < ST_RUN )
	{
		SET_CUR_FUNC( &ug_clipper::process_all_stop );
		ResetStaticOutput();
	}
	else
	{
		SET_CUR_FUNC( &ug_clipper::sub_process );
	}

	GetPlug(PN_OUT)->TransmitState( p_clock, out_stat );
}

// TODO version with fixed clip levels !!!
void ug_clipper::sub_process(int start_pos, int sampleframes)
{
	float* input = input_ptr + start_pos;
	float* output = output_ptr + start_pos;
	float* hi_limit = hi_lim_ptr + start_pos;
	float* lo_limit = lo_lim_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		if( *input > *hi_limit )
			*output = *hi_limit;
		else
		{
			if( *input < *lo_limit )
				*output = *lo_limit;
			else
				*output = *input;
		}

		input++;
		output++;
		hi_limit++;
		lo_limit++;
	}
}

void ug_clipper::process_all_stop(int start_pos, int sampleframes)
{
	float* input = input_ptr + start_pos;
	float* output = output_ptr + start_pos;
	float* hi_limit = hi_lim_ptr + start_pos;
	float* lo_limit = lo_lim_ptr + start_pos;
	float out;

	if( *input > *hi_limit )
		out = *hi_limit;
	else
	{
		if( *input < *lo_limit )
			out = *lo_limit;
		else
			out = *input;
	}

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*output++ = out;
	}

	SleepIfOutputStatic(sampleframes);
}


