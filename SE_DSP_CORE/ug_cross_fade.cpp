#include "pch.h"
#include <algorithm>
#include "ug_cross_fade.h"

#include "ug_pan.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_cross_fade)

using namespace std;

namespace
{
REGISTER_MODULE_1_BC(36,L"X-Mix", IDS_MN_X_MIX,IDS_MG_MODIFIERS,ug_cross_fade ,CF_STRUCTURE_VIEW,L"Mixes two signals into one.");
}

#define TABLE_SIZE 512	// equal power curve lookup table

#define PLG_SIGNAL1		0
#define PLG_SIGNAL2		1
#define PLG_MIX			2
#define PLG_OUTPUT		3
#define PLG_CURVE_TYPE	4

// Fill an array of InterfaceObjects with plugs and parameters
void ug_cross_fade::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Template, ppactive, Default,Range/enum list
	LIST_PIN2( L"Input A",input1_ptr, DR_IN, L"0", L"",IO_LINEAR_INPUT, L"");
	LIST_PIN2( L"Input B",input2_ptr, DR_IN, L"0", L"",IO_LINEAR_INPUT, L"");
	LIST_PIN2( L"Mix",pan_ptr, DR_IN, L"0", L"5,-5,5,-5",IO_POLYPHONIC_ACTIVE, L"Varies the mix from +5V - Input A, to -5V - Input B");
	LIST_PIN2( L"Signal Out",left_output_ptr, DR_OUT, L"5", L"",0, L"");
	//	LIST_VAR3( L"Fade Law", fade_type, DR _PARAMETER, DT_ENUM   , L"1", L"Equal Intensity (0dB), Equal Power(+3dB), Sqr Root", 0, L"Chooses different cross fade laws");
	LIST_VAR3( L"Fade Law", fade_type, DR_IN, DT_ENUM   , L"1", L"Equal Intensity (0dB), Equal Power(+3dB), Sqr Root", IO_MINIMISED, L"Chooses different cross fade laws");
}

ug_cross_fade::ug_cross_fade()
{
}

// This is called when the input changes state
void ug_cross_fade::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	if( p_to_plug == GetPlug(PLG_CURVE_TYPE) )
	{
		SetLookupTable();
	}

	if( p_to_plug == GetPlug(PLG_MIX) || p_to_plug == GetPlug(PLG_CURVE_TYPE) )
	{
		if( p_state < ST_RUN )
		{
			CalcFixedMultipliers(GetPlug( PLG_MIX )->getValue());
		}
	}

	state_type out_stat = max( ST_ONE_OFF, max( GetPlug(PLG_SIGNAL1)->getState(), GetPlug(PLG_SIGNAL2)->getState() ));

	if( GetPlug(PLG_MIX)->getState() == ST_RUN )
	{
		if( fade_type == 0 ) // linear
		{
			SET_CUR_FUNC( &ug_cross_fade::sub_process_mix_linear );
		}
		else
		{
			SET_CUR_FUNC( &ug_cross_fade::sub_process_mix );
		}

		out_stat = ST_RUN;
	}
	else
	{
		if( out_stat == ST_RUN )
		{
			SET_CUR_FUNC( &ug_cross_fade::sub_process );
		}
		else
		{
			ResetStaticOutput();
			SET_CUR_FUNC( &ug_cross_fade::sub_process_static );
		}
	}

	OutputChange( p_clock, GetPlug(PLG_OUTPUT), out_stat );
}

void ug_cross_fade::sub_process_static(int start_pos, int sampleframes)
{
	sub_process(start_pos, sampleframes);
	SleepIfOutputStatic(sampleframes);
}

void ug_cross_fade::sub_process(int start_pos, int sampleframes)
{
	float* input1 = input1_ptr + start_pos;
	float* input2 = input2_ptr + start_pos;
	float* output = left_output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*output++ = *input1++ * multiplier_r + *input2++ * multiplier_l;
	}
}

void ug_cross_fade::sub_process_mix(int start_pos, int sampleframes)
{
	float* input1 = input1_ptr + start_pos;
	float* input2 = input2_ptr + start_pos;
	float* mix = pan_ptr + start_pos;
	float* output = left_output_ptr + start_pos;
	//const float table_size = TABLE_SIZE;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float idx_l = 0.5f - *mix++;

/*
		idx_l = max( idx_l, 0.f );
		idx_l = min( idx_l, 1.0f );
		multiplier_l = shared_lookup_table->GetValueInterp2(idx_l);
		multiplier_r = shared_lookup_table->GetValueInterp2(1.f-idx_l);
*/
		multiplier_l = shared_lookup_table->GetValueInterpClipped(idx_l);
		multiplier_r = shared_lookup_table->GetValueInterpClipped(1.f-idx_l);
		

		*output++ = *input1++ * multiplier_r + *input2++ * multiplier_l;
	}
}

void ug_cross_fade::sub_process_mix_linear(int start_pos, int sampleframes)
{
	float* input1 = input1_ptr + start_pos;
	float* input2 = input2_ptr + start_pos;
	float* mix = pan_ptr + start_pos;
	float* output = left_output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float idx_l = 0.5f - *mix++;
		idx_l = max( idx_l, 0.f );
		idx_l = min( idx_l, 1.f );
		*output++ = *input2++ * idx_l + *input1++ * (1.f-idx_l);
	}
}
