#include "pch.h"
#include <algorithm>
#include "ug_filter_biquad.h"

#include "ULookup.h"
#include <math.h>
#include <float.h>
#include <mutex>
#include "conversion.h"
#include "SeAudioMaster.h"
#include "module_register.h"
#include "./modules/shared/xplatform.h"

SE_DECLARE_INIT_STATIC_FILE(ug_filter_biquad)

using namespace std;

namespace
{
REGISTER_MODULE_1(L"Moog Filter", IDS_MN_MOOG_FILTER, IDS_MG_FILTERS,ug_filter_biquad ,CF_STRUCTURE_VIEW,L"A very 'Fat' 4 pole low pass filter.  Has built in overdrive if pushed too far. Has higher CPU load than the SV Filter.");
}

#define TABLE_SIZE 512

// signals below this level are due to self-oscilation
#define INSIGNIFICANT_SAMPLE 0.0000015f

#define PN_SIGNAL		0
#define PN_PITCH		1
#define PN_RESON		2
#define PN_OUTPUT		3
#define PN_MODE			4
#define PN_BOOST		5
#define PN_FREQ_SCALE	6

int trash;
float* trash2;

// Fill an array of InterfaceObjects with plugs and parameters
void ug_filter_biquad::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_PIN2( L"Signal",input_ptr,			DR_IN, L"0", L"", IO_LINEAR_INPUT, L"");
	LIST_PIN2( L"Pitch",pitch_ptr,		DR_IN, L"5", L"", IO_POLYPHONIC_ACTIVE, L"Controls the Filter's 'Cutoff'. 1 Volt per Octave or 1 Volt / kHz switchable");
	LIST_PIN2( L"Resonance",resonance_ptr,		DR_IN, L"5", L"", IO_POLYPHONIC_ACTIVE, L"Emphasises the Cutoff frequency");
	LIST_PIN2( L"Output",output_ptr,		DR_OUT, L"0", L"", 0, L"");
	//	LIST_VAR3( L"Mode", mode,     DR_IN, DT_ENUM   , L"0", L"4 Pole LP,Low Shelf, High Shelf, Peaking EQ", 0, L"");
	LIST_VAR3( L"Mode", trash,     DR_IN, DT_ENUM   , L"0", L"4 Pole LP", IO_DISABLE_IF_POS, L"Not used at present");
	LIST_PIN2( L"Boost/Cut",trash2,		DR_IN, L"0", L"10,-10,10,-10", IO_DISABLE_IF_POS|IO_POLYPHONIC_ACTIVE, L"For Parametric Filters, +ve = boost, -ve = cut. NOT USED AT PRESENT");
	//	LIST_VAR3( L"Freq Scale", freq_scale,	DR _PARAMETER, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", 0, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
	LIST_VAR3( L"Freq Scale", freq_scale,	DR_IN, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", IO_POLYPHONIC_ACTIVE|IO_MINIMISED, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
}

ug_filter_biquad::ug_filter_biquad() :
	output_quiet(true)
{
}

int ug_filter_biquad::Open()
{
	// fix for race conditions.
	static std::mutex safeInit;
	std::lock_guard<std::mutex> lock(safeInit);

	// This must happen b4 first stat change arrives
	CreateSharedLookup2( L"Moog LP Scale", lookup_table, (int) getSampleRate(), TABLE_SIZE + 2, true, SLS_ALL_MODULES );
	CreateSharedLookup2( L"Moog LP kp", lookup_table2, (int) getSampleRate(), TABLE_SIZE + 2, true, SLS_ALL_MODULES );

	// Fill lookup tables if not done already
	if( !lookup_table->GetInitialised() )
	{
		//lookup_table->SetSize(TABLE_SIZE + 2);
		//lookup_table2->SetSize(TABLE_SIZE + 2);

		for( int j = 0 ; j < lookup_table->GetSize(); j++ )
		{
			float freq_hz = VoltageToFrequency( 10.f * (float)j / (float)TABLE_SIZE );
			freq_hz = min(freq_hz, getSampleRate() / 2.f); // help prevent numeric overflow when temp mult by MAX_SAMPLE
			float fcon = 2.f * freq_hz / getSampleRate();		// normalized freq. 0 to Nyquist * /
			kp      = 3.6f * fcon - 1.6f * fcon * fcon - 1.f;	// Emperical tuning              * /
			pp1d2   = (kp + 1.f) * 0.5f;						// Timesaver                     * /
			float temp_scale = expf(( 1.f - pp1d2) * 1.386249f ); // Scaling factor  * /
			lookup_table->SetValue( j, temp_scale );
			lookup_table2->SetValue( j, kp );
		}

		lookup_table->SetInitialised();
	}

	ug_base::Open();
	SET_CUR_FUNC( &ug_filter_biquad::process_all );
	//	OnModeChange();
	// moog init
	xnm1 = y1nm1 = y2nm1 = y3nm1 = 0.f;
	y1n  = y2n   = y3n   = y4n   = 0.f;
	// para eq init
	// The equalizer filter is initialized to zero.
	xnm1 = xnm2 = ynm2 = 0.f;
	return 0;
}

/* too hard to determin inital conditions due to feedback etc
void ug_filter_biquad::OnFirstSample()
{
	// setting the filter to a steady state
	// we can save CPU in the case that it has a flat input signal by
	// allowing the steady-state chaek to trigger earlier

	float inital_value = 0.f;
	if( GetPlug(PN_SIGNAL)->getState() < ST_RUN )
	{
		//freq_change( float p_pitch, float p_resonance );
		// approximate stable values
		inital_value = GetPlug(PN_SIGNAL)->getValue();
	}

	// moog init
	xnm1 = y1nm1 = y2nm1 = y3nm1 = inital_value;
	y1n  = y2n   = y3n   = y4n   = inital_value;

	// para eq init
	// The equalizer filter is initialized to zero.
	xnm1 = xnm2 = ynm2 = inital_value;
}
*/
void ug_filter_biquad::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	if( p_to_plug == GetPlug(PN_RESON) || p_to_plug == GetPlug(PN_PITCH) || p_to_plug == GetPlug(PN_FREQ_SCALE))
	{
		freq_change( GetPlug(PN_PITCH)->getValue(), GetPlug( PN_RESON )->getValue() );
	}

	if( p_to_plug == GetPlug(PN_SIGNAL) )
	{
		GetPlug(PN_OUTPUT)->TransmitState( SampleClock(), ST_RUN );
	}

	if( GetPlug(PN_PITCH)->getState() == ST_RUN || GetPlug(PN_RESON)->getState() == ST_RUN)
	{
		SET_CUR_FUNC( &ug_filter_biquad::process_all );
	}
	else
	{
		if( GetPlug(PN_SIGNAL)->getState() == ST_RUN )
		{
			SET_CUR_FUNC( &ug_filter_biquad::process_fixed_freq );
		}
		else
		{
			// special version with extra denormal removal
			SET_CUR_FUNC( &ug_filter_biquad::process_no_input );
		}
	}

	// this is done after process func set, as stability check might set sleep mode
	if( output_quiet )
	{
		output_quiet = false;
//		RUN_AT( SampleClock() + 50, &ug_filter_biquad::OverflowCheck ); // check faily soon in hope of stable result
	}
}

void ug_filter_biquad::process_fixed_freq(int start_pos, int sampleframes)
{
	doStabilityCheck();

	float* in		= input_ptr + start_pos;
	float* out	= output_ptr + start_pos;
	constexpr float one_sixth = 1.f / 6.f;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float xn = *in++ - k * y4n; // Inverted feed back for corner peaking
		// Four cascaded onepole filters (bilinear transform)
		y1n   = (xn + xnm1)   * pp1d2 - kp * y1n;
		y2n   = (y1n + y1nm1) * pp1d2 - kp * y2n;
		y3n   = (y2n + y2nm1) * pp1d2 - kp * y3n;
		y4n   = (y3n + y3nm1) * pp1d2 - kp * y4n;
		// Clipper band limited sigmoid (really only valid for x < root 6 =2.45)
		int pre_clip = *((int*) &y4n); // get sign before clipping
		y4n   = y4n - y4n * y4n * y4n * one_sixth;

		// convoluted (but fast) way of checking if sign has flipped
		if( (pre_clip ^ *((int*) &y4n)) < 0 ) // means clipping becoming unstable (energy added to filter)
		{
			y4n = 0.f;
		}

		xnm1  = xn;		// Update Xn-1
		y1nm1 = y1n;	// Update Y1n-1
		y2nm1 = y2n;	// Update Y2n-1
		y3nm1 = y3n;	// Update Y3n-1
		//kill_denormal(y4n);
		*out++ = y4n;
	}

	// even if you remove denormals from stage4,
	// they will still circulate independantly in stage 1,2, and 3
	//kill_denormal(xnm1);
}

// when input is switched off while running, energy exists in filter
// this will quickly denormalise and filter gets stuck in high CPU denormal mode
// denormals can get stuck in all of the 4 stages.  It's no use just clearing one.
// also assumes freq input is fixed.
void ug_filter_biquad::process_no_input(int start_pos, int sampleframes)
{
	doStabilityCheck();

	float* in		= input_ptr + start_pos;
	float* out	= output_ptr + start_pos;
	constexpr float one_sixth = 1.f / 6.f;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float xn = *in++ - k * y4n; // Inverted feed back for corner peaking
		// Four cascaded onepole filters (bilinear transform)
		y1n   = (xn + xnm1)   * pp1d2 - kp * y1n;
		//kill_denormal(y1n);
		y2n   = (y1n + y1nm1) * pp1d2 - kp * y2n;
		//kill_denormal(y2n);
		y3n   = (y2n + y2nm1) * pp1d2 - kp * y3n;
		//kill_denormal(y3n);
		y4n   = (y3n + y3nm1) * pp1d2 - kp * y4n;
		// Clipper band limited sigmoid (really only valid for x < root 6 =2.45)
		int pre_clip = *((int*) &y4n); // get sign before clipping
		y4n   = y4n - y4n * y4n * y4n * one_sixth;

		// convoluted (but fast) way of checking if sign has flipped
		if( (pre_clip ^ *((int*) &y4n)) < 0 ) // means clipping becoming unstable (energy added to filter)
		{
			y4n = 0.f;
		}

		xnm1  = xn;		// Update Xn-1
		y1nm1 = y1n;	// Update Y1n-1
		y2nm1 = y2n;	// Update Y2n-1
		y3nm1 = y3n;	// Update Y3n-1
		//kill_denormal(y4n);
		*out++ = y4n;
	}

	// even if you remove denormals from stage4,
	// they will still circulate independantly in stage 1,2, and 3
	//kill_denormal(xnm1);
}

void ug_filter_biquad::process_all(int start_pos, int sampleframes)
{
	doStabilityCheck();

	float* in		= input_ptr + start_pos;
	float* pitch	= pitch_ptr + start_pos;
	float* resonance= resonance_ptr + start_pos;
	float* out	= output_ptr + start_pos;
	constexpr float one_sixth = 1.f / 6.f;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		freq_change( *pitch++, *resonance++ );
		float xn = *in++ - k * y4n; // Inverted feed back for corner peaking
		// Four cascaded onepole filters (bilinear transform)
		y1n   = (xn + xnm1)   * pp1d2 - kp * y1n;
		y2n   = (y1n + y1nm1) * pp1d2 - kp * y2n;
		y3n   = (y2n + y2nm1) * pp1d2 - kp * y3n;
		y4n   = (y3n + y3nm1) * pp1d2 - kp * y4n;
		// Clipper band limited sigmoid
		int pre_clip = *((int*) &y4n); // get sign before clipping
		y4n   = y4n - y4n * y4n * y4n * one_sixth;

		// convoluted (but fast) way of checking if sign has flipped
		if( (pre_clip ^ *((int*) &y4n)) < 0 ) // means clipping becoming unstable (energy added to filter)
		{
			y4n = 0.f;
		}

		xnm1  = xn;		// Update Xn-1
		y1nm1 = y1n;	// Update Y1n-1
		y2nm1 = y2n;	// Update Y2n-1
		y3nm1 = y3n;	// Update Y3n-1
		//kill_denormal(y4n);
		*out++ = y4n;
	}
}

/* not used
void ug_filter_biquad::process_static(int start_pos, int sampleframes)
{
	assert( output_quiet );

	float *out = output_ptr + start_pos;

	int todo = sampleframes;
	if( todo > static_output_count )
		todo = static_output_count;

	for( int s = todo ; s > 0 ; s-- )
	{
		*out++ = y4n;
	}

	static_output_count -= todo;

	if( static_output_count <= 0 )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
//		GetPlug(PN_OUTPUT)->TransmitState( SampleClock(), ST_STOP );
	}
}
*/
void ug_filter_biquad::freq_change( float p_pitch, float p_resonance )
{
	if( p_pitch < 0 || freq_scale != 0 )	// do it the hard way
	{
		float freq_hz;

		// convert freq to Hz
		if( freq_scale == 0 ) // 1 Volt per Octave
		{
			freq_hz = VoltageToFrequency( 10.f * p_pitch );
		}
		else	// 1 Volt = 1 kHz
		{
			freq_hz = 10000.f * p_pitch;
		}

		freq_hz = min( freq_hz, getSampleRate()  * 0.495f); // limit to 1/2 sample rate
		// scale could be lookup ( kp,pp1d2 not so expensive )
		float fcon = 2.f * freq_hz / (float)getSampleRate();		// normalized freq. 0 to Nyquist
		kp      = 3.6f * fcon - 1.6f * fcon * fcon - 1.f;	// Emperical tuning
		pp1d2 = (kp + 1.f) * 0.5f;
		scale   = expf(( 1.f - pp1d2) * 1.386249f ); // Scaling factor
	}
	else
	{
		if( p_pitch > 1.f )
		{
			p_pitch = 1.f;
		}

		kp	= lookup_table2->GetValueInterp2( p_pitch );
		pp1d2 = (kp + 1.f) * 0.5f;
		scale = lookup_table->GetValueInterp2( p_pitch );
	}

	k = p_resonance * scale;
}

void ug_filter_biquad::freq_change_1pole()
{
	// a0 = something
}

void ug_filter_biquad::StabilityCheck()
{
	// periodic check/correct for numeric overflow
	if( !isfinite(y4n) ) // overload?
	{
		y1n = y2n = y3n = y4n = xnm1 = y1nm1 = y2nm1 = y3nm1 = 0.f;
	}

	// run next check in 1 second
//	int next_check_delay = (int) SampleRate();

	// check for output quiet (means we can sleep)
	if( output_quiet == false ) // input stopped
	{
		if( GetPlug(PN_SIGNAL)->getState() != ST_RUN ) // input stopped
		{
			// input is stable, check more often
//			next_check_delay = AudioMaster()->BlockSize() * 4;
			// this filter can get stuck with small alternating output values
			// approx 1 e -35. clip such values to zero.
			// must set at least these 3 to zero, else denormals keep circulating
			bool clamped_output_to_zero = false;

			if( fabsf(xnm1) < INSIGNIFICANT_SAMPLE )
			{
				xnm1 = 0.f;

				if( fabsf(y4n) < INSIGNIFICANT_SAMPLE && y4n != 0.f && GetPlug(PN_SIGNAL)->getValue() == 0.f )
				{
					// output may be stuck at some very small value, however, better to clamp to zero, then next time round sleep module
					// Allows downstream VCA to see it's input is zero and sleep.
					clamped_output_to_zero = true;
					y4n = 0.f;
				}

				if( fabsf(y1n) < INSIGNIFICANT_SAMPLE )
				{
					y1n = 0.f;
				}

				if( fabsf(y2n) < INSIGNIFICANT_SAMPLE )
				{
					y2n = 0.f;
				}

				if( fabsf(y3n) < INSIGNIFICANT_SAMPLE )
				{
					y3n = 0.f;
				}
			}

			if( clamped_output_to_zero == false )
			{
				// check that output buffer is stable (all values the same)
				float* output_buffer = GetPlug(PN_OUTPUT)->GetSamplePtr();
				float out_val = *output_buffer;
				int i;

				for( i = AudioMaster()->BlockSize() ; i > 0 ; i-- )
				{
					if( !(*output_buffer++ == out_val) )
					{
						break;
					}
				}

				if( i == 0 )
				{
					output_quiet = true;
					GetPlug(PN_OUTPUT)->TransmitState( SampleClock(), ST_STOP );
					SET_CUR_FUNC( &ug_base::process_sleep );
					return;
				}
			}

			/*
			float input = GetPlug(PN_SIGNAL)->getValue();

			// rough estimate of energy in filter
			//			float energy = fabs(y3n - y2n) + fabs(y2n - y1n);
			float xn = input - k * y4n; // Inverted feed back for corner peaking
			float energy = fabs(y3n - y3nm1) + fabs(xn - xnm1);
			if( energy < INSIGNIFICANT_SAMPLE )
			{
				// no. it's clipped too. y4n = input;
				output_quiet = true;
				ResetStaticOutput();
				GetPlug(PN_OUTPUT)->TransmitState( SampleClock(), ST_STOP );
				SET_CUR_FUNC( process_static );
				return;
			}
			*/
		}

//		RUN_AT( SampleClock() + next_check_delay, &ug_filter_biquad::OverflowCheck );
	}
}

/*
The Zolzer biquads are calculated in the same way as rbj's. Both take an analog
prototype (s plane) and use the Bilinear transform to map it to the z plane. The
only difference (assuming starting with the same analog prototype) is that
Zolzer keeps the tan and rbj breaks it down to sin and cos using a trig identity
(tanx = sinx/cosx, or tax(x/2) = (1-cosx)/sinx). (If you use a math program to
simplify the equations for you, you might also come up with things in terms of
sin/cos as it does the substitution.)

In a nutshell, you start with an analog prototype (say, 1/(s+1)). You replace
's" with (1-z^-1)/(1+z^-1). Because you're trying to map the continuous s domain
onto the circular z domain, you need to warp it to squeeze the top end into the
finite available space, so you multiply by 1/tan(omega/2), where omega is the
frequency of interest in radian, or simply put, 2 * pi * Fc / Fs (Fc is
frequency of the filter, Fs is sample rate). So, you end upreplacing s with
1/tan(pi*Fc/Fs)*(1-z^-1)/(1+z^-1). Then the fun begins--you collect the terms
and simplify. It helps to have a math package to cut down the job. I like to
keep things in terms of tan, like Zolzer, so I'll go with 1/k*(1-z^-1)/(1+z^-1)
and solve k=tan(pi*Fc/Fs) later.

You end up with things looking like this (if you try ;-) :

			a0 + a1*z^-1 + a2*z^-2
	H(z) = -------------------------
			b0 + b1*z^-1 + b2*z^-2

Now, some people like to use a's for the top coefs (Zolzer, Rabiner), and some
the b's rbj & others)--there is no "standard". It doesn't matter as provided
you're not confused. To implement it, you shuffle that into a difference
equation, which ends up:

a0*x + a1*x[n-1] + a2*x[n-2] - b0*y - b1*y[n-1] - b2*y[n-2] = 0

You really want to solve for the output, y, so,

y = (a0*x + a1*x[n-1] + a2*x[n-2] - b1*y[n-1] - b2*y[n-2]) / b0

or,

y = a0/b0*x + a1/b0*x[n-1] + a2/b0*x[n-2] - b1/b0*y[n-1] - b2/b0*y[n-2]

Naturally, you do all those divisions beforehand, and often the equations you
see already have that done (you'll see a0,a1,a2,b1,b2, all with the same
denominator, which is of course the b0). If you implement that equation directly
in a block diagram, you'll see the familiar "direct form I" biquad.

I tried to keep this short and to the point, not rigorous, so if anyone else
wants to add, have at it. I haven't seen a tutorial on the web for this sort of
thing, but there must be one somewhere. DSP texts tend to gloss over the
mechanics of this (or present it so rigorously that you lose sight what you need
to do), so if you're learning DSP on your own, you might find that it's a nice
feeling once you work a filter out for yourself and get the right answer.

Nigel
*/