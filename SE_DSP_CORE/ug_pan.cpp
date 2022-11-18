#include "pch.h"
#include <algorithm>
#include "ug_pan.h"

#include "ug_vca.h"
#include "ULookup.h"
#include "module_register.h"
#include <math.h>

SE_DECLARE_INIT_STATIC_FILE(ug_pan)

using namespace std;

namespace
{
REGISTER_MODULE_1_BC(37,L"Pan", IDS_MN_PAN,IDS_MG_MODIFIERS,ug_pan ,CF_STRUCTURE_VIEW,L"Panorama module. Provides Left/Right and Volume control of a signal.");
}


#define TABLE_SIZE 512	// equal power curve lookup table

#define PLG_INPUT		0
#define PLG_PAN			1
#define PLG_LEFT_OUT	2
#define PLG_RIGHT_OUT	3
#define PLG_VOLUME		4
#define PLG_CURVE_TYPE	5

// Fill an array of InterfaceObjects with plugs and parameters
void ug_pan::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Template, ppactive, Default,Range/enum list
	LIST_PIN2(L"Input", input1_ptr, DR_IN, L"0", L"",IO_LINEAR_INPUT, L"Audio signal input");
	LIST_PIN2(L"Pan", pan_ptr, DR_IN, L"0", L"5,-5,5,-5",IO_POLYPHONIC_ACTIVE, L"-5V = Left, +5V = Right");
	LIST_PIN2(L"Left Out",left_output_ptr, DR_OUT, L"5", L"",0, L"");
	LIST_PIN2(L"Right Out",right_output_ptr, DR_OUT, L"5", L"",0, L"");
	LIST_PIN2(L"Volume",volume_ptr, DR_IN,  L"8",L"", IO_POLYPHONIC_ACTIVE,L"Controls the Volume of the input signal. 10 V = original volume, 0 V = silence");
	LIST_VAR3(L"Fade Law", fade_type, DR_IN, DT_ENUM   ,L"1",L"Equal Intensity (0dB), Equal Power(+3dB), Sqr Root", IO_MINIMISED, L"Chooses different cross fade laws");
}

ug_pan::ug_pan()
{
}

// This is called when the input changes state
void ug_pan::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	if( p_to_plug == GetPlug(PLG_CURVE_TYPE))
	{
		SetLookupTable();
	}

	if( p_to_plug == GetPlug(PLG_VOLUME) || p_to_plug == GetPlug(PLG_PAN) || p_to_plug == GetPlug(PLG_CURVE_TYPE))
	{
		if( p_state < ST_RUN )
		{
			CalcFixedMultipliers( GetPlug( PLG_PAN )->getValue() );
			// calc overall gain
			float gain = CalcGain( GetPlug( PLG_VOLUME )->getValue() );
			multiplier_l *= gain;
			multiplier_r *= gain;
		}
	}

	state_type out_stat = max( GetPlug(PLG_INPUT)->getState(), GetPlug(PLG_PAN)->getState() );
	out_stat = max( out_stat, GetPlug(PLG_VOLUME)->getState() );

	// if input is stopped and zero, can sleep
	if( GetPlug(PLG_INPUT)->getState() < ST_RUN && GetPlug(PLG_INPUT)->getValue() == 0.f )
	{
		out_stat = ST_STATIC;
	}

	if( GetPlug(PLG_VOLUME)->getState() < ST_RUN && GetPlug(PLG_VOLUME)->getValue() == 0.f )
	{
		out_stat = ST_STATIC;
	}

	state_type out_stat_l = out_stat;
	state_type out_stat_r = out_stat;

	if( GetPlug(PLG_PAN)->getState() == ST_RUN || GetPlug(PLG_VOLUME)->getState() == ST_RUN)
	{
		if( fade_type == 0 ) // linear
		{
			SET_CUR_FUNC( &ug_pan::sub_process_pan_linear );
		}
		else
		{
			SET_CUR_FUNC( &ug_pan::sub_process_pan );
		}
	}
	else
	{
		if( GetPlug(PLG_INPUT)->getState() == ST_RUN )
		{
			SET_CUR_FUNC( &ug_pan::sub_process );

			// If panned hard one side, set other side st_static.
			if( multiplier_l == 0.0f )
			{
				out_stat_l = ST_STATIC;
			}

			if( multiplier_r == 0.0f )
			{
				out_stat_r = ST_STATIC;
			}
		}
		else
		{
			ResetStaticOutput();
			SET_CUR_FUNC( &ug_pan::sub_process_static );
		}
	}

	if( out_stat < ST_RUN )
	{
		ResetStaticOutput();
		SET_CUR_FUNC( &ug_pan::sub_process_static );
	}

	OutputChange( p_clock, GetPlug(PLG_LEFT_OUT), out_stat_l );
	OutputChange( p_clock, GetPlug(PLG_RIGHT_OUT), out_stat_r );
}

void ug_pan::sub_process_static(int start_pos, int sampleframes)
{
	assert( GetPlug(PLG_INPUT)->getState() < ST_RUN || (multiplier_l == 0.f && multiplier_r == 0.f ));
	sub_process(start_pos, sampleframes);
	SleepIfOutputStatic(sampleframes);
}

void ug_pan::sub_process(int start_pos, int sampleframes)
{
	float* input = input1_ptr + start_pos;
	float* output_left = left_output_ptr + start_pos;
	float* output_right = right_output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*output_left++ = *input * multiplier_l;
		*output_right++ = *input++ * multiplier_r;
	}
}

void ug_pan::sub_process_pan(int start_pos, int sampleframes)
{
	float* input = input1_ptr + start_pos;
	float* pan = pan_ptr + start_pos;
	float* volume = volume_ptr + start_pos;
	float* output_left = left_output_ptr + start_pos;
	float* output_right = right_output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		// calc overall gain
		float gain = CalcGain( *volume++ ); // expands inline
		// calc left and right levels
		float idx_l = 0.5f - *pan++;
		idx_l = max( idx_l, 0.f );
		idx_l = min( idx_l, 1.f );
		multiplier_l = shared_lookup_table->GetValueInterp2(idx_l) * gain;
		multiplier_r = shared_lookup_table->GetValueInterp2(1.f - idx_l) * gain;

		*output_left++ = *input * multiplier_l;
		*output_right++ = *input++ * multiplier_r;
	}
}

void ug_pan::sub_process_pan_linear(int start_pos, int sampleframes)
{
	float* input = input1_ptr + start_pos;
	float* pan = pan_ptr + start_pos;
	float* volume = volume_ptr + start_pos;
	float* output_left = left_output_ptr + start_pos;
	float* output_right = right_output_ptr + start_pos;
//	const float table_size = TABLE_SIZE;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		// calc overall gain
		float gain = CalcGain( *volume++ ); // expands inline
		// calc left and right levels
		float idx_l = 0.5f - *pan++;
		idx_l = max( idx_l, 0.f );
		idx_l = min( idx_l, 1.f );
		*output_left++ = *input * idx_l * gain;
		*output_right++ = *input++ * (1.f-idx_l) * gain;
	}
}

int ug_pan::Open()
{
	// Create volume curve lookup table (also see UXFade)
	InitFadeTables( this, shared_lookup_table_sin, shared_lookup_table_sqr  );
	ug_vca::InitVolTable( this, shared_lookup_table_db );	// borrow VCA's volume curve
	SetLookupTable();
	return ug_base::Open();
}

// this ug should be cloned per note
// if the audio in is from the voice
// and if the pan or volume depends on the note number (downstrream from notesource)
// if the volume or pan is determined by sliders, no need to clone this
bool ug_pan::PPGetActiveFlag()
{
	bool level_depends_on_note = GetPlug(PLG_VOLUME)->GetPPDownstreamFlag() | GetPlug(PLG_PAN)->GetPPDownstreamFlag();
	// UVCA is special.  Only active if BOTH inputs are active
	return GetPlug(PLG_INPUT)->GetPPDownstreamFlag() && level_depends_on_note;
}

void ug_pan::CalcFixedMultipliers(float p_pan)
{
	// calc left and right levels
	float idx_l = 0.5f - p_pan;
	idx_l = max( idx_l, 0.f );
	idx_l = min( idx_l, 1.f );

	if( fade_type == 0 ) // linear
	{
		multiplier_l = idx_l;
		multiplier_r = 1.f - idx_l;
	}
	else
	{
		multiplier_l = shared_lookup_table->GetValueInterp2(idx_l);
		multiplier_r = shared_lookup_table->GetValueInterp2(1.f - idx_l);
	}
}
/*
Software packages now often have a selector for 3 or 6db panning law.


	The +3dB notch comes from the concept that uncorrelated signals, i.e.
those with no real common components, will simply add as rms power.
So if you want to add two equal uncorrelated signals (which includes
pink and white noise) and still maintain the same rms level as each
signal by itself, you need to notch each only 3dB.  The problem I have
with this (and your mileage may vary) is that panpots split a signal
into two channels, so *by definition* they are correlated!  For
panpots then, a 3dB notch seems not to make much sense unless you
*want* the common material common to pick up that 3dB when hear in
mono.  By comparison, the Mix control of a Dry and Wet signal on a
reverb unit, for example, is generally dealing with two different
signals which have very little common material.  Hence a 3dB notch is
the right one for that application.



The point being that 6db law is the natural thing in panpots, while 3db
law is the natural thing in crossfaders. Nonetheless, many people like
the 3db law center boost in panpots as it subjectively increases the
panning effect for listeners outside the sweet spot - but I've never met
anybody who thought 6db law in crossfaders desirable.

  Dunno about Mackie, but the old (and very quiet) Studer compact
console was about -4.5 dB at center. I suspect there are many other
examples. A pan-potted stereo signal is obviously not L/R
decorrelated, because it started out as one or more mono signals. But
it can never be perfectly correlated either, unless you're listening
in an anechoic chamber.

#define PAN_LAW_ATTENUATION (-3.0)

float scale = 2.0 - 4.0 * powf(10,PAN_LAW_ATTENUATION/20.0);
float panL = pan, panR = 1-pan;
float gainL = scale * panL*panL + (1.0 - scale) * panL;
float gainR = scale * panR*panR + (1.0 - scale) * panR;

*/

void ug_pan::InitFadeTables(ug_base* p_ug, ULookup * &sin_table, ULookup * &sqr_table)
{
	p_ug->CreateSharedLookup2( L"Equal Power Curve", sqr_table, -1, TABLE_SIZE+2, true, SLS_ALL_MODULES );

	if( !sqr_table->GetInitialised() )
	{
		for( int j = 0 ; j < TABLE_SIZE + 2; j++ )
		{
			float temp_float = sqrtf( j / (float)TABLE_SIZE );
			sqr_table->SetValue( j, temp_float );
		}

		sqr_table->SetInitialised();
	}

	p_ug->CreateSharedLookup2( L"Sin Equal Power Curve", sin_table, -1, TABLE_SIZE+2, true, SLS_ALL_MODULES );

	if( !sin_table->GetInitialised() )
	{
		for( int j = 0 ; j < TABLE_SIZE + 2; j++ )
		{
			float temp_float = sinf( 0.5f * PI * (float)j / (float)TABLE_SIZE );
			sin_table->SetValue( j, temp_float );
		}

		sin_table->SetInitialised();
	}
}
/*
Equal-power panning uses the same method, but uses the square root of the master amplitude control value for each channel
or is it sin(x) vs Cos(x)?
*/

void ug_pan::SetLookupTable()
{
	if( fade_type == 1 )
		shared_lookup_table = shared_lookup_table_sin;
	else
		shared_lookup_table = shared_lookup_table_sqr;
}
