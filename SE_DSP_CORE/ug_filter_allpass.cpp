#include "pch.h"
#include <algorithm>
#include "ug_filter_allpass.h"

#include "ULookup.h"
#include <math.h>
#include <float.h>
#include "conversion.h"
#include "module_register.h"
#include "./modules/shared/xplatform.h"

SE_DECLARE_INIT_STATIC_FILE(ug_filter_allpass)

using namespace std;

namespace
{
REGISTER_MODULE_1(L"All Pass", IDS_MN_ALL_PASS,IDS_MG_FILTERS,ug_filter_allpass ,CF_STRUCTURE_VIEW,L"A two-pole all-pass filter.");
}

#define TABLE_SIZE 512

// signals below this level are due to self-oscilation
#define INSIGNIFICANT_SAMPLE 0.0000015f

#define PN_SIGNAL		0
#define PN_PITCH		1
#define PN_RESON		2
#define PN_OUTPUT		3
#define PN_FREQ_SCALE	4

// Fill an array of InterfaceObjects with plugs and parameters
void ug_filter_allpass::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_PIN2( L"Signal",	input_ptr,	DR_IN,L"0", L"", IO_LINEAR_INPUT, L"");
	LIST_PIN2( L"Pitch",pitch_ptr,		DR_IN,L"5", L"", IO_POLYPHONIC_ACTIVE, L"Controls the Filter's 'Cutoff'. 1 Volt per Octave");
	LIST_PIN2( L"Resonance",resonance_ptr,	DR_IN,L"5", L"", IO_POLYPHONIC_ACTIVE, L"Controls the rollover frequency band width");
	LIST_PIN2( L"Output",	output_ptr,	DR_OUT,L"0", L"", 0, L"");
	//	LIST_VAR3( L"Freq Scale", freq_scale,     DR _PARAMETER, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", 0, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
	LIST_VAR3( L"Freq Scale", freq_scale,	DR_IN, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", IO_POLYPHONIC_ACTIVE|IO_MINIMISED, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
}

void ug_filter_allpass::freq_change( float p_pitch, float p_resonance )
{
	// q limited to range 0.5 - 2
	float q_const = 1.5f * p_resonance + 0.5f;
	q_const = max(q_const, 0.5f );
	q_const = min(q_const, 4.f );
	//	q_const = 1.0 / q_const;
	float freq_hz;

	// convert freq to Hz
	if( freq_scale == 0 ) // 1 Volt per Octave
	{
		freq_hz = VoltageToFrequency( 10.f * p_pitch );
	}
	else	// 1 Volt = 1 kHz
	{
		freq_hz = 10000.f * p_pitch;
		freq_hz = max( freq_hz, 0.f );
	}

	float f = freq_hz / getSampleRate();
	f = min( f, 0.5f ); // limit to 1/2 sample rate

	if( p_pitch < 0 )	// do it the hard way
	{
		// scale could be lookup ( kp,pp1d2 not so expensive )
		factor_A = 2.f * cos( PI2 * f ) * exp( - PI * f *q_const ) ;
		factor_B = exp( - PI2 * f * q_const );
	}
	else
	{
		//		float idx2 = f * (float)TABLE_SIZE;
		//		float idx = idx2 * q_const;
		//		idx = min( idx, TABLE_SIZE );
		//		factor_A = lookup_tableB->GetValue Interpolated( idx2 ) * lookup_tableA->GetValue Interpolated( 0.5 * idx );
		//		factor_B = lookup_tableA->GetValue Interpolated( idx ); // lookup exp()
		float idx = f * q_const;
		idx = min( idx, 1.f );
		factor_A = lookup_tableB->GetValueInterp2( f ) * lookup_tableA->GetValueInterp2( 0.5f * idx );
		factor_B = lookup_tableA->GetValueInterp2( idx ); // lookup exp()
#ifdef _DEBUG	// ENSURE LOOKUP TABLE WORKING
		float factor_A_exact = 2.f * cos( PI2 * f ) * exp( - PI * f * q_const );
		assert( fabs(( factor_A_exact - factor_A ) / factor_A_exact ) < 0.01f );
		float factor_B_exact = exp( 2.f * (- PI * f * q_const) );
		assert( fabs(( factor_B_exact - factor_B ) / factor_B_exact ) < 0.01f );
#endif
	}
}

int ug_filter_allpass::Open()
{
	CreateSharedLookup2( L"ug_filter_allpass A", lookup_tableA, -1, TABLE_SIZE + 2, true, SLS_ALL_MODULES );
	CreateSharedLookup2( L"ug_filter_allpass B", lookup_tableB, -1, TABLE_SIZE + 2, true, SLS_ALL_MODULES );

	// Fill lookup tables if not done already
	if( !lookup_tableA->GetInitialised() )
	{
		//lookup_tableA->SetSize(TABLE_SIZE + 2);
		//lookup_tableB->SetSize(TABLE_SIZE + 2);

		for( int j = 0 ; j < lookup_tableA->GetSize() ; j++ )
		{
			float temp = expf( - PI2 * j / (float)TABLE_SIZE );
			lookup_tableA->SetValue( j, temp );
			temp = 2.f * cosf( PI2 * j / (float)TABLE_SIZE );
			lookup_tableB->SetValue( j, temp );
		}

		lookup_tableA->SetInitialised();
		lookup_tableB->SetInitialised();
	}

	ug_base::Open();
	SET_CUR_FUNC( &ug_filter_allpass::process_all );
	delay2 = delay1 = 0.0f;
	return 0;
}


void ug_filter_allpass::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	if( p_to_plug == GetPlug(PN_RESON) || p_to_plug == GetPlug(PN_PITCH) || p_to_plug == GetPlug(PN_FREQ_SCALE))
	{
		freq_change( GetPlug(PN_PITCH)->getValue(), GetPlug( PN_RESON )->getValue() );
	}

	if( p_to_plug == GetPlug(PN_SIGNAL) )
	{
		GetPlug(PN_OUTPUT)->TransmitState( SampleClock(), ST_RUN );
	}

	if( GetPlug(PN_PITCH)->getState() == ST_RUN || GetPlug(PN_RESON)->getState() == ST_RUN )
	{
		SET_CUR_FUNC( &ug_filter_allpass::process_all );
	}
	else
	{
		SET_CUR_FUNC( &ug_filter_allpass::process_fixed_freq );
	}

	// this is done after process func set, as stability check might set sleep mode
	if( output_quiet ) // any input change requires restart of overflow check
	{
		output_quiet = false;
		OverflowCheck();
	}
}

void ug_filter_allpass::process_fixed_freq(int start_pos, int sampleframes)
{
	float* in	= input_ptr + start_pos;
	float* out= output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float temp = delay1 * factor_A;
		float temp3 = *in++ + temp - delay2 * factor_B;
		*out++ = temp3 * factor_B + delay2 - temp;
		delay2 = delay1;
		delay1 = temp3;
	}
}

void ug_filter_allpass::process_all(int start_pos, int sampleframes)
{
	float* in		= input_ptr + start_pos;
	float* pitch	= pitch_ptr + start_pos;
	float* resonance= resonance_ptr + start_pos;
	float* out	= output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		freq_change( *pitch++, *resonance++ );
		float temp = delay1 * factor_A;
		float temp3 = *in++ + temp - delay2 * factor_B;
		*out++ = temp3 * factor_B + delay2 - temp;
		delay2 = delay1;
		delay1 = temp3;
	}
}

void ug_filter_allpass::process_static(int start_pos, int sampleframes)
{
	assert( output_quiet );
	float* out	= output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = delay1;
	}

	static_output_count -= sampleframes;

	if( static_output_count <= 0 )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
		GetPlug(PN_OUTPUT)->TransmitState( SampleClock() + sampleframes, ST_STOP );
	}
}

void ug_filter_allpass::OverflowCheck()
{
	//	_RPT1(_CRT_WARN, "ug_filter_sv::OverflowCheck() %d\n", SampleClock() );
	// periodic check/correct for numeric overflow
	if( ! isfinite( delay1 ) ) // overload?
	{
		delay1 = 0.f;
	}

	if( ! isfinite( delay2 ) ) // overload?
	{
		delay2 = 0.f;
	}

	// check for output quiet (means we can sleep)
	if( output_quiet == false )
	{
		if( GetPlug(PN_SIGNAL)->getState() != ST_RUN ) // input stopped
		{
			float input = GetPlug(PN_SIGNAL)->getValue();
			// rough estimate of energy in filter. Difficult to use as non-zero fixed inputs produce signifigant 'enery' due to numeric errors
			float energy = fabsf(delay1 - delay2);
			// new method, stop filter when roundoff error results in output getting stuck
			// needed with fixed input voltage
			float temp = delay1 * factor_A;
			float temp3 = input + temp - delay2 * factor_B;
			//kill_denormal(temp3);

			// temp3 is next value of delay1
			if( temp3 == delay1 || energy < INSIGNIFICANT_SAMPLE ) // y1n = current output
			{
				delay1 = input;
				delay2 = input;
				output_quiet = true;
				ResetStaticOutput();
				SET_CUR_FUNC( &ug_filter_allpass::process_static );
				return;
			}
		}

		RUN_AT( SampleClock() + (int) getSampleRate(), &ug_filter_allpass::OverflowCheck );
	}
}
