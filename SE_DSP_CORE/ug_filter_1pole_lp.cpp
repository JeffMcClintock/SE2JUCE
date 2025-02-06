#include "pch.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include "ug_filter_1pole_lp.h"
#include <mutex>
#include "ULookup.h"
#include "conversion.h"
#include "module_register.h"
#include "./modules/shared/xplatform.h"

SE_DECLARE_INIT_STATIC_FILE(ug_filter_1pole)
SE_DECLARE_INIT_STATIC_FILE(ug_filter_1pole_hp)

using namespace std;

// keywords one pole
namespace
{
REGISTER_MODULE_1(L"1 Pole LP", IDS_MN_1_POLE_LP,IDS_MG_FILTERS,ug_filter_1pole ,CF_STRUCTURE_VIEW,L"A simple, efficient Low Pass filter. Has 6db/Octave response.");
REGISTER_MODULE_1(L"1 Pole HP", IDS_MN_1_POLE_HP,IDS_MG_FILTERS,ug_filter_1pole_hp ,CF_STRUCTURE_VIEW,L"A simple, efficient Hi Pass filter. Has 6db/Octave response.");
}

#define TABLE_SIZE 512
// signals below this level are due to self-oscilation
#define INSIGNIFICANT_SAMPLE 0.000001f

// once input stops, filter is periodically monitored to
// wait for output to die down
#define AUTO_SHUTOFF_TIME 40

#define PN_SIGNAL		0
#define PN_PITCH		1
#define PN_OUTPUT		2
#define PN_FREQ_SCALE	3

// Fill an array of InterfaceObjects with plugs and parameters
void ug_filter_1pole::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_PIN2( L"Signal",input_ptr,		DR_IN, L"0", L"", IO_LINEAR_INPUT, L"");
	LIST_PIN2( L"Pitch",pitch_ptr,		DR_IN, L"5", L"", IO_POLYPHONIC_ACTIVE, L"Controls the Filter's 'Cutoff'. 1 Volt per Octave or 1 Volt / kHz switchable");
	LIST_PIN2( L"Output",output_ptr,		DR_OUT, L"0", L"", 0, L"");
	//	LIST_VAR3( L"Freq Scale", freq_scale,	DR _PARAMETER, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", IO_POLYPHONIC_ACTIVE, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
	LIST_VAR3( L"Freq Scale", freq_scale,	DR_IN, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", IO_POLYPHONIC_ACTIVE|IO_MINIMISED, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
}

// Hi Pass exactly the same as low-pass except this...
ug_filter_1pole_hp::ug_filter_1pole_hp()
{
	low_pass_mode = false;
}

ug_filter_1pole::ug_filter_1pole() :
	low_pass_mode(true)
	,output_quiet(true)
{
}

int ug_filter_1pole::Open()
{
	// fix for race conditions.
	static std::mutex safeInit;
	std::lock_guard<std::mutex> lock(safeInit);

	// This must happen b4 first stat change arrives
	RUN_AT( SampleClock(), &ug_filter_1pole::OnFirstSample );
	CreateSharedLookup2( L"1 pole l", lookup_table, (int) getSampleRate(), TABLE_SIZE + 2, true, SLS_ALL_MODULES );

	// Fill lookup tables if not done already
	if( !lookup_table->GetInitialised() )
	{
//		lookup_table->SetSize(TABLE_SIZE + 2);
		float const1 = - PI2 / getSampleRate();

		for( int j = 0 ; j < lookup_table->GetSize(); j++ )
		{
			float freq_hz = VoltageToFrequency( 10.f * (float)j / (float)TABLE_SIZE );
			freq_hz = min( freq_hz, getSampleRate()  * 0.495f); // limit to 1/2 sample rate
			l = exp( freq_hz * const1 ); // approximation !!!

			// TODO: !!! use the *exact* freq calculation (here and in process) !!!
			// https://dsp.stackexchange.com/questions/54086/single-pole-iir-low-pass-filter-which-is-the-correct-formula-for-the-decay-coe
#if 0 //def _DEBUG
			{
				const auto y = 1.0 - cos(PI2 * freq_hz / getSampleRate());
				const auto alpha = -y + sqrt(y * y + 2.0 * y); // exact 1 - 'l'

				_RPTN(0, "%f, %f\n", l, 1.0 - alpha);
			}
#endif

			lookup_table->SetValue( j, l );
		}

		lookup_table->SetInitialised();
	}

	ug_base::Open();
/*
	if( low_pass_mode )
	{
		SET_CUR_FUNC( &ug_filter_1pole::process_all_lp );
	}
	else
	{
		SET_CUR_FUNC( &ug_filter_1pole::process_all_hp );
	}
*/

	// randomise stabilityCheckCounter to avoid CPU spikes.
	stabilityCheckCounter = 63 & m_handle;

	y1n = 0.f;
	return 0;
}

void ug_filter_1pole::OnFirstSample()
{
	// Often a filter is disabled via switch, or just filters a control signal.
	// setting the filter to a steady state
	// we can save CPU in the case that it has a flat input signal by
	// allowing the steady-state check to trigger first time/
	// stable value
	if( GetPlug(PN_SIGNAL)->getState() < ST_RUN )
	{
		y1n = GetPlug(PN_SIGNAL)->getValue();
	}
}

void ug_filter_1pole::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	if( p_to_plug == GetPlug(PN_PITCH) || p_to_plug == GetPlug(PN_FREQ_SCALE))
	{
		freq_change( GetPlug(PN_PITCH)->getValue() );
	}

	if( p_to_plug == GetPlug(PN_SIGNAL) )
	{
		GetPlug(PN_OUTPUT)->TransmitState( SampleClock(), ST_RUN );
	}

	if( GetPlug(PN_PITCH)->getState() == ST_RUN )
	{
		if( low_pass_mode )
		{
			SET_CUR_FUNC( &ug_filter_1pole::process_all_lp );
		}
		else
		{
			SET_CUR_FUNC( &ug_filter_1pole::process_all_hp );
		}
	}
	else
	{
		bool settling = GetPlug(PN_SIGNAL)->getState() != ST_RUN;

		if( low_pass_mode )
		{
			if( settling )
			{
				SET_CUR_FUNC( &ug_filter_1pole::process_fixed_freq_lp_settling );
			}
			else
			{
				SET_CUR_FUNC( &ug_filter_1pole::process_fixed_freq_lp );
			}
		}
		else
		{
			if( settling )
			{
				SET_CUR_FUNC( &ug_filter_1pole::process_fixed_freq_hp_settling );
			}
			else
			{
				SET_CUR_FUNC( &ug_filter_1pole::process_fixed_freq_hp );
			}
		}
	}

	if( output_quiet )
	{
		output_quiet = false;
	}
}

void ug_filter_1pole::process_fixed_freq_lp_settling(int start_pos, int sampleframes)
{
	doStabilityCheck();

	process_fixed_freq_lp( start_pos, sampleframes );

	assert( GetPlug(PN_SIGNAL)->getState() != ST_RUN );

	float input = input_ptr[start_pos];
	// rough estimate of energy in filter
	float energy = fabs(input-y1n);
	// stop filter when roundoff error results in output getting stuck
	float next_output = input + l * ( y1n - input );

	if( next_output == y1n || energy < INSIGNIFICANT_SAMPLE ) // y1n = current output
	{
		y1n = input;
		output_quiet = true;
		ResetStaticOutput();
		SET_CUR_FUNC( &ug_filter_1pole::process_static );
	}
}

void ug_filter_1pole::process_fixed_freq_hp_settling(int start_pos, int sampleframes)
{
	doStabilityCheck();

	process_fixed_freq_hp( start_pos, sampleframes );

	assert( GetPlug(PN_SIGNAL)->getState() != ST_RUN );

	float input = input_ptr[start_pos];
	// rough estimate of energy in filter
	float energy = fabs(input-y1n);
	// stop filter when roundoff error results in output getting stuck
	float next_output = l * ( y1n - input );

	if( next_output == 0.0f || energy < INSIGNIFICANT_SAMPLE )
	{
		y1n = input;
		output_quiet = true;
		ResetStaticOutput();
		SET_CUR_FUNC( &ug_filter_1pole::process_static );
	}
}

void ug_filter_1pole::process_fixed_freq_lp(int start_pos, int sampleframes)
{
	doStabilityCheck();

	float* in	= input_ptr + start_pos;
	float* out= output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float xn = *in++;
		// low pass
		*out++ = y1n = xn + l * ( y1n - xn );
	}
}

void ug_filter_1pole::process_all_lp(int start_pos, int sampleframes)
{
	doStabilityCheck();

	float* in		= input_ptr + start_pos;
	float* pitch	= pitch_ptr + start_pos;
	float* out	= output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		freq_change( *pitch++ );
		float xn = *in++;
		// low pass
		//y1n = k * xn + l * y1n; // k = 0..1
		y1n = xn + l * ( y1n - xn );
		*out++ = y1n;
	}
}

void ug_filter_1pole::process_fixed_freq_hp(int start_pos, int sampleframes)
{
	doStabilityCheck();

	float* in	= input_ptr + start_pos;
	float* out= output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float xn = *in++;
		y1n = xn + l * ( y1n - xn );
		*out++ = xn - y1n;
	}
}

void ug_filter_1pole::process_all_hp(int start_pos, int sampleframes)
{
	doStabilityCheck();

	float* in		= input_ptr + start_pos;
	float* pitch	= pitch_ptr + start_pos;
	float* out	= output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		freq_change( *pitch++ );
		float xn = *in++;
		y1n = xn + l * ( y1n - xn );
		*out++ = xn - y1n;
	}
}

void ug_filter_1pole::process_static(int start_pos, int sampleframes)
{
	assert( output_quiet );
	float* out = output_ptr + start_pos;
	float c = 0.f;

	if( low_pass_mode )
	{
		c = y1n;
	}

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = c;
	}

	static_output_count -= sampleframes;

	if( static_output_count <= 0 )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
		GetPlug(PN_OUTPUT)->TransmitState( SampleClock() + sampleframes, ST_STOP );
	}
}

void ug_filter_1pole::freq_change( float p_pitch )
{
	// Within normal range use lookup table.
	if( p_pitch >= 0.0f && p_pitch <= 1.0f && freq_scale == 0 )
	{
		l = lookup_table->GetValueInterp2( p_pitch );
	}
	else
	{
		float freq_hz;

		if( freq_scale == 0 ) // 1 Volt per Octave
		{
			freq_hz = VoltageToFrequency( 10.f * p_pitch );
		}
		else	// 1 Volt = 1 kHz
		{
			freq_hz = 10000.f * p_pitch;
		}

		freq_hz = min( freq_hz, getSampleRate() * 0.495f); // limit to 1/2 sample rate
		l = exp( -PI2 * freq_hz / getSampleRate()); // approximation
	}

	k = 1.f - l;
}

void ug_filter_1pole::StabilityCheck()
{
	// periodic check/correct for numeric overflow
	if( !isfinite(y1n) ) // overload?
	{
		y1n = 0.f;
	}
}