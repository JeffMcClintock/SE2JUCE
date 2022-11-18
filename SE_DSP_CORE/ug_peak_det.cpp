#include "pch.h"
#include <algorithm>
#include "ug_peak_det.h"

#include <math.h>
#include "conversion.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_peak_det)


using namespace std;


#define INSIGNIFICANT_SAMPLE 0.000001f

namespace
{
REGISTER_MODULE_1_BC(41,L"Peak Follower", IDS_MN_PEAK_FOLLOWER,IDS_MG_MODIFIERS,ug_peak_det ,CF_STRUCTURE_VIEW,L"The output attempts to follow the envelope (level) of the input signal.");
}

// prevent divide by zero
#define MIN_ATTACK 0.001

#define PN_INPUT	0
#define PN_ATTACK	1
#define PN_DECAY	2
#define PN_OUTPUT	3

// Fill an array of InterfaceObjects with plugs and parameters
void ug_peak_det::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Template, ppactive, Default,Range/enum list
	LIST_PIN2( L"Signal in",input_ptr, DR_IN, L"0", L"", IO_POLYPHONIC_ACTIVE, L"Input signal");
	LIST_PIN2( L"Attack",attack_ptr, DR_IN, L"8", L"", IO_POLYPHONIC_ACTIVE, L"Rate at which current_level rises to track input level");
	LIST_PIN2( L"Decay",decay_ptr, DR_IN, L"7", L"", IO_POLYPHONIC_ACTIVE, L"Rate at which current_level falls to track input level");
	LIST_PIN2( L"Signal Out",output_ptr, DR_OUT, L"0", L"", 0, L"current_level Signal");
}

ug_peak_det::ug_peak_det() :
	current_level(0.f)
{
}

int ug_peak_det::Open()
{
	ug_base::Open();
//	OutputChange( SampleClock(), GetPlug(PN_OUTPUT), ST_RUN ); // needs proper shut-off when inactive
	return 0;
}

/* music DSP code. log10(0.01) = -4.6 ish
attack_coef = exp(log(0.01)/( attack_in_ms * samplerate * 0.001));
release_coef = exp(log(0.01)/( release_in_ms * samplerate * 0.001));

to reverse coef: ms = -1000 / (log(coef) * sample-rate);

envelope = 0.0;

loop::

tmp = fabs(in);
if(tmp > envelope)
	envelope = attack_coef * (envelope - tmp) + tmp;
else
	envelope = release_coef * (envelope - tmp) + tmp;
*/

// this means: ms = volts * 20
// 1 Volt = 20ms

// This is called when the input changes state
void ug_peak_det::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type /*p_state*/)
{
	state_type input1_status = GetPlug( PN_INPUT )->getState();
	state_type input2_status = GetPlug( PN_ATTACK )->getState();
	state_type input3_status = GetPlug( PN_DECAY )->getState();
	state_type out_stat = max( input1_status, input2_status );
	out_stat = max( out_stat, input3_status );
	int in = p_to_plug->getPlugIndex();

	if( in == PN_ATTACK )
	{
		ga = GetPlug( PN_ATTACK )->getValue();
		ga = max( ga, MIN_ATTACK );
		ga = exp(-100.0/(getSampleRate() * ga));
		// _RPT2(_CRT_WARN, "%f -> %f\n", GetPlug( PN_ATTACK )->getValue(),ga );
	}

	if( in == PN_DECAY )
	{
		gr = GetPlug( PN_DECAY )->getValue();
		gr = max( gr, MIN_ATTACK );
		gr = exp(-100.0/(getSampleRate() * gr ));
	}

	if( input2_status < ST_RUN && input3_status < ST_RUN )
	{
		ResetStaticOutput();

		if( input1_status < ST_RUN )
		{
			SET_CUR_FUNC(&ug_peak_det::sub_process_settling);
		}
		else
		{
			SET_CUR_FUNC( &ug_peak_det::sub_process );
		}
	}
	else
	{
		SET_CUR_FUNC( &ug_peak_det::sub_process_all_run );
	}

	GetPlug(PN_OUTPUT)->TransmitState( p_clock, ST_RUN );
}

void ug_peak_det::sub_process(int start_pos, int sampleframes)
{
	float* in = input_ptr + start_pos;
	float* out = output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float i = *in++;
		i = i < 0.f ? -i : i; // fabs()

		if(current_level < i)
			current_level = ga*(current_level-i) + i;
		else
			current_level = gr*(current_level-i) + i;

		*out++ = (float) current_level;
	}
}

void ug_peak_det::sub_process_settling(int start_pos, int sampleframes)
{
	float* in = input_ptr + start_pos;
	float* out = output_ptr + start_pos;

	double i = (double) *in;
	i = i < 0.0 ? -i : i; // fabs()

	for (int s = sampleframes; s > 0; s--)
	{
		if (current_level < i)
			current_level = ga*(current_level - i) + i;
		else
			current_level = gr*(current_level - i) + i;

		*out++ = (float) current_level;
	}

	if ( fabs(current_level-i) < INSIGNIFICANT_SAMPLE ) // y1n = current output
	{
		ResetStaticOutput();
		SET_CUR_FUNC(&ug_peak_det::process_static);
	}
}

void ug_peak_det::process_static(int start_pos, int sampleframes)
{
	float* in = input_ptr + start_pos;
	float* out = output_ptr + start_pos;

	float c = fabsf(*in);

	for (int s = sampleframes; s > 0; s--)
	{
		*out++ = c;
	}

	static_output_count -= sampleframes;

	if (static_output_count <= 0)
	{
		SET_CUR_FUNC(&ug_base::process_sleep);
		GetPlug(PN_OUTPUT)->TransmitState(SampleClock() + sampleframes, ST_STOP);
	}
}


void ug_peak_det::sub_process_all_run(int start_pos, int sampleframes)
{
	float* in = input_ptr + start_pos;
	float* attack = attack_ptr + start_pos;
	float* decay = decay_ptr + start_pos;
	float* out = output_ptr + start_pos;

	const float neg100_over_samplerate = -100.0f / getSampleRate();

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		double lga = (double) *attack++;
		lga = max( lga, MIN_ATTACK );
		lga = exp( neg100_over_samplerate / lga );
		double lgr = (double) *decay++;
		lgr = max( lgr, MIN_ATTACK );
		lgr = exp( neg100_over_samplerate / lgr );

		float i = *in++;
		i = i < 0.f ? -i : i; // fabs()

		if(current_level < i)
			current_level = lga*(current_level-i) + i;
		else
			current_level = lgr*(current_level-i) + i;

		*out++ = (float) current_level;
	}
}
/*
	SAMPLE v = abs(*input);

	if( v > current_level )
		current_level += attack_increment;
	else
	{
		current_level -= decay_increment;
		if( current_level <= v )
		{
			current_level = v;
			if( !input_running )
			{
				run_function( (Pug_func) tick, ST_STOP );
				current_level_change( current_level, ST_STOP );
			}
		}
	}

	current_level = current_level;
*/
// from music dsp
/*
void ug_peak_det::OnRateChange_attack()
{
	double temp_float = TimecentToSecond(VoltageToTimecent(SampleToVoltage(*attack)));

	// convert to a rate in units/ sample
	temp_float *= sample_rate;
	if( temp_float < 1 )  // prevent divide by 0
		temp_float = 1;
	temp_float = ( MAX_LONG/2 ) / temp_float;
	temp_float /= 8;
	attack_increment = temp_float;
}

void ug_peak_det::OnRateChange_decay()
{
	double temp_float = TimecentToSecond(VoltageToTimecent(SampleToVoltage(*decay)));

	// convert to a rate in units/ sample
	temp_float *= sample_rate;
	if( temp_float < 1 )  // prevent divide by 0
		temp_float = 1;
	temp_float = ( MAX_LONG/2 ) / temp_float;
	temp_float /= 70;
	decay_increment = temp_float;
}*/
/*
I run into many troubles a decade ago trying to do a flute to MIDI converter
with standard frequency measuring techniques. I think the best way is an
estimator. I've collected a couple of articles based on FFT,
autocorrelation, constant-Q transform, and a couple that claimed to be the
best based on least-squares fitting techniques. I didn't try any of them.
IIRC, the best working methods usually do an FFT and then, instead of
detecting the stronger frequency or the lowest peak, try to find the
harmonic relation among the strongest three peaks; this works OK even with
"fundamental-less" sounds.

Search for "Real time fundamental frequency estimation by least-square
fitting" by Andrew Choi at the University of Hong Kong.
*/