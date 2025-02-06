#include "pch.h"
#include <algorithm>
#include <mutex>
#include "ug_filter_sv.h"

#include "ULookup.h"
#include <math.h>
#include <float.h>
#include "conversion.h"
#include "module_register.h"
#include "SeAudioMaster.h"
#include "./modules/shared/xplatform.h"

SE_DECLARE_INIT_STATIC_FILE(ug_filter_sv)

using namespace std;

namespace
{
REGISTER_MODULE_1(L"SV Filter", IDS_MN_SV_FILTER,IDS_MG_OLD,ug_filter_sv ,CF_STRUCTURE_VIEW,L"'State Variable' Filter. Changes the frequency content of the sound.  Provides Low Pass ( only low frequencies pass through), High Pass, Band Pass, and Band Reject outputs.  The resonance control adds a peak to the response (great for dance music bass lines). 2 pole, 12 db/ octave response");
}

/* from synthmaker website
// taylor series version of 2 * sin(x)
x = 3.141592 * (cutoff / 36.0) * 2 * 3.141592;
x2 = x*x;
x3 = x2*x;
x4 = x3*x;
x5 = x4*x;
x6 = x5*x;
x7 = x6*x;
f = 2.0 * (x - (x3 / 6.0) + (x5 / 120.0) - ( x7 / 5040.0));

*/

#define TABLE_SIZE 512

// signals below this level are due to self-oscilation
#define INSIGNIFICANT_SAMPLE 0.000001f

#define PN_SIGNAL		0
#define PN_PITCH		1
#define PN_RESON		2
#define PN_LOW_PASS		3
#define PN_HI_PASS		4
#define PN_BAND_PASS	5
#define PN_BAND_REJECT	6
#define PN_FREQ_SCALE	7

// Fill an array of InterfaceObjects with plugs and parameters
void ug_filter_sv::ListInterface2(InterfaceObjectArray& PList)
{
	// IO Var, Direction, Datatype, Name, Default, defid (index into ::PlugFormats)
	// defid used to name a enum list or range of values
	LIST_PIN2( L"Signal",	input_ptr,	DR_IN,L"0", L"", IO_LINEAR_INPUT, L"");
	LIST_PIN2( L"Pitch",pitch_ptr,	DR_IN,L"5", L"", IO_POLYPHONIC_ACTIVE, L"Controls the Filter's 'Cutoff'. 1 Volt per Octave");
	LIST_PIN2( L"Resonance",resonance_ptr,	DR_IN,L"5", L"", IO_POLYPHONIC_ACTIVE, L"Emphasises the Cutoff frequency");
	LIST_PIN2( L"Low Pass",lp_ptr,	DR_OUT,L"0", L"", 0, L"");
	LIST_PIN2( L"Hi Pass",hp_ptr,		DR_OUT,L"0", L"", 0, L"");
	LIST_PIN2( L"Band Pass",bp_ptr,	DR_OUT,L"0", L"", 0, L"");
	LIST_PIN2( L"Band Reject",br_ptr,	DR_OUT,L"0", L"", 0, L"");
	//	LIST_VAR3( L"Freq Scale", freq_scale,     DR _PARAMETER, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", 0, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
	LIST_VAR3( L"Freq Scale", freq_scale,	DR_IN, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", IO_POLYPHONIC_ACTIVE|IO_MINIMISED, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
}

#define UNINITIALISED_FLOAT_VALUE (1234567.89f)

ug_filter_sv::ug_filter_sv() :
	band_pass(0.f)
	,low_pass(0.f)
	,max_stable_freq(UNINITIALISED_FLOAT_VALUE)
	,fixed_quality_factor(UNINITIALISED_FLOAT_VALUE)
	,output_quiet(true)
    ,factor1(UNINITIALISED_FLOAT_VALUE)
{
}

/*
Subject: RE: [music-dsp] Correct way to oversample a ring modulator?
Date: Wed, 8 Oct 2003 19:30:08 +0100
Reply-To: music-dsp@ceait.calarts.edu

I can recommend the half band filters on the music-dsp website -
http://www.musicdsp.org/archive.php?classid=3#39

which are designed for up & down sampling and use polyphase allpass filters.
Their attenuation above fs/2 is comparible with using FIR filters but
because their IIR, their much quicker to run.

Regards
Rob


*/
int ug_filter_sv::Open()
{
#if defined( _DEBUG ) && defined( _MSC_VER )
	unsigned int fpState;
	_controlfp_s( &fpState, 0, 0 ); // flush-denormals-to-zero mode?
	assert( (fpState & _MCW_DN) == _DN_FLUSH && "err: caller needs to set flush denormals mode during Open()." );
#endif

	// fix for race conditions.
	static std::mutex safeInit;
	std::lock_guard<std::mutex> lock(safeInit);

	// This must happen b4 first stat change arrives
	RUN_AT( SampleClock(), &ug_filter_sv::OnFirstSample );
	CreateSharedLookup2( L"ug_filter_sv Cutoff", lookup_table, (int)getSampleRate(), TABLE_SIZE + 2, true, SLS_ALL_MODULES );
	CreateSharedLookup2( L"ug_filter_sv Resonance Limit", max_freq_lookup, -1, TABLE_SIZE + 2, true, SLS_ALL_MODULES );

	// Fill lookup tables if not done already
	if( !lookup_table->GetInitialised() )
	{
//		lookup_table->SetSize(TABLE_SIZE + 2);

		for( int j = 0 ; j < lookup_table->GetSize(); j++ )
		{
			float freq_hz = VoltageToFrequency( 10.f * (float)j / (float)TABLE_SIZE );
			freq_hz = min(freq_hz, getSampleRate()/2); // help prevent numeric overflow when temp mult by MAX_SAMPLE
			float temp = 2.f * PI * freq_hz / getSampleRate();
			lookup_table->SetValue( j, temp );
		}

		lookup_table->SetInitialised();
	}

	m_lookup_table_data = lookup_table->GetBlock();

	// init max freq lookup table (given reson, lookup max freq filter can handle)
	if( !max_freq_lookup->GetInitialised() )
	{
//		max_freq_lookup->SetSize(TABLE_SIZE + 2);

		for( int j = 0 ; j < max_freq_lookup->GetSize() ; j++ )
		{
			// orig, too harsh			max_freq_lookup->SetValue( j,VoltageToSample( 0.000025 * j*j + 0.009 * j + 8.2378) );
			float temp = (0.000025f * j*j + 0.009f * j + 7.8f);
			max_freq_lookup->SetValue( j, temp * 0.1f );
		}

		max_freq_lookup->SetInitialised();
	}

	ug_base::Open();
	SET_CUR_FUNC( &ug_filter_sv::process_all );
	only_lp_used = false;
	only_hp_used = false;

	if( !GetPlug(PN_BAND_PASS)->InUse() && !GetPlug(PN_BAND_REJECT)->InUse() )
	{
		if( !GetPlug(PN_HI_PASS)->InUse() )
		{
			only_lp_used = true;
		}

		if( !GetPlug(PN_LOW_PASS)->InUse() )
		{
			only_hp_used = true;
		}
	}

	return 0;
}

void ug_filter_sv::OnFirstSample()
{
	// Often a filter is disabled via switch, or just filters a control signal.
	// setting the filter to a steady state
	// we can save CPU in the case that it has a flat input signal by
	// allowing the steady-state check to trigger first time/
	// stable value
	if( GetPlug(PN_SIGNAL)->getState() < ST_RUN )
	{
		low_pass = GetPlug(PN_SIGNAL)->getValue();
	}
}

void ug_filter_sv::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
#if defined( _DEBUG ) && defined( _MSC_VER )
	unsigned int fpState;
	_controlfp_s( &fpState, 0, 0 ); // flush-denormals-to-zero mode?
	assert( (fpState & _MCW_DN) == _DN_FLUSH && "err: caller needs to set flush denormals mode during Process()." );
#endif

	if( p_to_plug == GetPlug(PN_RESON) || p_to_plug == GetPlug(PN_PITCH) || p_to_plug == GetPlug(PN_FREQ_SCALE))
	{
		if( p_state < ST_RUN ) // avoid unnesc calc (sometimes)
		{
			fixed_quality_factor = CalcQ( GetPlug( PN_RESON )->getValue() );
			max_stable_freq = CalcMSF( GetPlug( PN_RESON )->getValue() );
			assert( max_stable_freq < PI ); // limit to < nyquist
			factor1 = CalcFreqFactor( GetPlug( PN_PITCH )->getValue() );
			// if either pitch or resonance change, limit frequency factor
			factor1 = min( factor1, max_stable_freq );
		}
	}

	if( p_to_plug == GetPlug(PN_SIGNAL) )
	{
		SetOutputState( SampleClock(), ST_RUN );
	}

	if( GetPlug(PN_RESON)->getState() == ST_RUN )
	{
		SET_CUR_FUNC( &ug_filter_sv::process_all );
	}
	else
	{
		// decide which process routine to use
		if( only_lp_used ) // and resonance fixed
		{
			if( GetPlug(PN_PITCH)->getState() == ST_RUN )
			{
				//			_RPT1(_CRT_WARN, "process_both_run %d\n", SampleClock() );
				SET_CUR_FUNC( &ug_filter_sv::process_both_run );
			}
			else
			{
				SET_CUR_FUNC( &ug_filter_sv::process_fixed_lp );
				//			_RPT1(_CRT_WARN, "process_fixed_lp %d\n", SampleClock() );
			}
		}
		else
		{
			if( only_hp_used && GetPlug(PN_PITCH)->getState() != ST_RUN) // and resonance fixed
			{
				SET_CUR_FUNC( &ug_filter_sv::process_fixed_hp );
				//			_RPT1(_CRT_WARN, "process_fixed_lp %d\n", SampleClock() );
			}
			else
			{
				SET_CUR_FUNC( &ug_filter_sv::process_all );
			}
		}
	}

	// this is done after process func set, as stability check might set sleep mode
	if( output_quiet )
	{
		output_quiet = false;
		OverflowCheck();
	}
}

// full processing, all outputs provided
void ug_filter_sv::process_all(int start_pos, int sampleframes)
{
	float* in		= input_ptr + start_pos;
	float* pitch	= pitch_ptr + start_pos;
	float* resonance= resonance_ptr + start_pos;
	float* lp		= lp_ptr + start_pos;
	float* hp		= hp_ptr + start_pos;
	float* bp		= bp_ptr + start_pos;
	float* br		= br_ptr + start_pos;
	float l_lp = low_pass;	// move into local vars for speed
	float l_bp = band_pass;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		// calc freq factor
		float f1;
		float idx = *pitch++;

		if( idx >= 0.f && freq_scale == 0 )
		{
			//float index = min( idx, 1.f );
			//f1 = fastInterpolation( index, TABLE_SIZE, m_lookup_table_data );
			f1 = fastInterpolationClipped( idx, TABLE_SIZE, m_lookup_table_data );
		}
		else
		{
			float freq_hz;// = VoltageToFrequency( idx * 10.f );

			if( freq_scale == 0 ) // 1 Volt per Octave
			{
				freq_hz = VoltageToFrequency( idx * 10.f );
			}
			else	// 1 Volt = 1 kHz
			{
				freq_hz = 10000.f * idx;
			}

			f1 = PI2 * freq_hz / getSampleRate();
		}

		// limit q to resonable values (under 9.992V)
		float q = *resonance++;
		q = max( q, 0.f );
		q = min( q, 0.9992f );
		float quality = 2.f - 2.f * q; // map to 0 - 2.0 range
		float l_max_stable_freq = max_freq_lookup->GetValueFloatNoInterpolation( (int)( q * 512.f) ); //!!float to int!! performance hit
		/*
				if( f1 > max _stable_freq )
					_RPT0(_CRT_WARN, "\nFilter:OVERLOAD");
				else
					_RPT0(_CRT_WARN, "\nFilter:OK");
		*/
		f1 = min( f1, l_max_stable_freq );
		l_lp += l_bp * f1;
		float hi_pass = *in++ - l_bp * quality - l_lp;
		l_bp += hi_pass * f1;
//		kill_denormal(l_bp);
		*lp++ = l_lp;
		*bp++ = l_bp;
		*hp++ = hi_pass;
		*br++ = hi_pass + l_lp;
	}

	// store delays
	low_pass = l_lp;
	band_pass = l_bp;
}

void ug_filter_sv::process_static(int start_pos, int sampleframes)
{
	assert( output_quiet );
	float* lp		= lp_ptr + start_pos;
	float* hp		= hp_ptr + start_pos;
	float* bp		= bp_ptr + start_pos;
	float* br		= br_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*hp++ = 0.0f;
		*lp++ = low_pass;
		*bp++ = 0.0f;;
		*br++ = 0.0f;
	}

	static_output_count -= sampleframes;

	if( static_output_count <= 0 )
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
		SetOutputState( SampleClock() + sampleframes, ST_STOP );
	}
}

// resonance fixed
// pitch, input is changing, only lp output used
void ug_filter_sv::process_both_run(int start_pos, int sampleframes)
{
	assert( fixed_quality_factor != UNINITIALISED_FLOAT_VALUE );
	assert( max_stable_freq != UNINITIALISED_FLOAT_VALUE );
	float* in		= input_ptr + start_pos;
	float* pitch	= pitch_ptr + start_pos;
	float* lp		= lp_ptr + start_pos;
	const float l_max_stable_freq = max_stable_freq; // moved to local var for speed
	float l_lp = low_pass;
	float l_bp = band_pass;
	const float quality = fixed_quality_factor;

	float f1;

	if( freq_scale == 0 ) // 1V/Octave. Reversed to handle infinite better
	{
		for( int s = sampleframes ; s > 0 ; s-- )
		{
			// calc freq factor
			float idx = *pitch++;

			if( idx >= 0.f )
			{
				//float index = min( idx, 1.f );
				//f1 = fastInterpolation( index, TABLE_SIZE, m_lookup_table_data );
				f1 = fastInterpolationClipped( idx, TABLE_SIZE, m_lookup_table_data );
			}
			else
			{
				float freq_hz = VoltageToFrequency( idx * 10.f );
				f1 = PI2 * freq_hz / getSampleRate();
			}

			f1 = min( f1, l_max_stable_freq );
			l_lp += l_bp * f1;
			l_bp += (*in++ - l_bp * quality - l_lp) * f1;
			//kill_denormal(l_bp);
			*lp++ = l_lp;
		}
	}
	else // 1 V/kHz
	{
		for( int s = sampleframes ; s > 0 ; s-- )
		{
			// calc freq factor
			float freq_hz = 10000.f * *pitch++;
			f1 = PI2 * freq_hz / getSampleRate();
			f1 = min( f1, l_max_stable_freq );
			l_lp += l_bp * f1;
			float hi_pass = *in++ - l_bp * quality - l_lp;
			l_bp += hi_pass * f1;
			//kill_denormal(l_bp);
			*lp++ = l_lp;
		}
	}

	// store delays
	low_pass = l_lp;
	band_pass = l_bp;
}

// only input changing, lp output only used
void ug_filter_sv::process_fixed_lp(int start_pos, int sampleframes)
{
	assert( fixed_quality_factor != UNINITIALISED_FLOAT_VALUE );
	assert( factor1 != UNINITIALISED_FLOAT_VALUE );
	float* in	= input_ptr + start_pos;
	float* lp	= lp_ptr + start_pos;
	float l_lp = low_pass;	// move into local vars for speed
	float l_bp = band_pass;
	const float f1 = factor1; // move to local var for speed

	for( int s = sampleframes ; s > 0 ; --s )
	{
		l_lp += l_bp * f1;
		float hi_pass = *in++ - l_bp * fixed_quality_factor - l_lp;
		l_bp += hi_pass * f1;
		*lp++ = l_lp;
	}

	// store delays
	low_pass = l_lp;
	band_pass = l_bp;
}

// only input changing, hp output only used
void ug_filter_sv::process_fixed_hp(int start_pos, int sampleframes)
{
	assert( fixed_quality_factor != UNINITIALISED_FLOAT_VALUE );
	assert( factor1 != UNINITIALISED_FLOAT_VALUE );
	float* in	= input_ptr + start_pos;
	float* hp	= hp_ptr + start_pos;
	float l_lp = low_pass;	// move into local vars for speed
	const float f1 = factor1;
	float l_bp = band_pass;
	const float q = fixed_quality_factor;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		l_lp += l_bp * f1;
		float hi_pass = *in++ - l_bp * q - l_lp;
		l_bp += hi_pass * f1;
		//kill_denormal(l_bp);
		*hp++ = hi_pass;
	}

	// store delays
	low_pass = l_lp;
	band_pass = l_bp;
}

void ug_filter_sv::OverflowCheck()
{
	//	_RPT1(_CRT_WARN, "ug_filter_sv::OverflowCheck() %d\n", SampleClock() );
	// periodic check/correct for numeric overflow
	if( ! isfinite( low_pass ) ) // overload?
	{
		low_pass = 0.f;
	}

	if( ! isfinite( band_pass ) ) // overload?
	{
		band_pass = 0.f;
	}

	// check for output quiet (means we can sleep)
	if( output_quiet == false )
	{
		if( GetPlug(PN_SIGNAL)->getState() != ST_RUN ) // input stopped
		{
			float input = GetPlug(PN_SIGNAL)->getValue();
			// rough estimate of energy in filter. Difficult to use as non-zero fixed inputs produce signifigant 'enery' due to numeric errors
			float energy = fabsf(low_pass - input) + fabsf(band_pass);
			// new method, stop filter when roundoff error results in output getting stuck
			// needed with fixed input voltage
			/*
						float f1 = CalcFreqFactor( GetPlug( PN_PITCH )->getValue() );
						float l_max_stable_freq = CalcMSF( GetPlug( PN_RESON )->getValue() );
						f1 = min( f1, l_max_stable_freq );
						float l_quality_factor = CalcQ( GetPlug( PN_RESON )->getValue() );

						float next_output = low_pass + band_pass * f1;
						float hi_pass = input - band_pass * l_quality_factor - next_output;
						float l_bp = band_pass + hi_pass * f1;
						//kill_denormal(l_bp);
						// ?? next_output += l_bp * f1;
			*/
			int blockPosition = getBlockPosition();
			// aviod 'mod' int previousBlockPosition = (blockPosition + AudioMaster()->BlockSize() - 1) % AudioMaster()->BlockSize();

			int previousBlockPosition = blockPosition - 1;
			if( previousBlockPosition < 0 )
			{
				previousBlockPosition = AudioMaster()->BlockSize() - 1;
			}

			float* buffer = GetPlug(PN_LOW_PASS)->GetSamplePtr();
			float Output = *( buffer + blockPosition );
			float previousOutput = *( buffer + previousBlockPosition );

			if( Output == previousOutput || energy < INSIGNIFICANT_SAMPLE )
			{
				low_pass = input;
				band_pass = 0.f;
				output_quiet = true;
				ResetStaticOutput();
				SET_CUR_FUNC( &ug_filter_sv::process_static );
				return;
			}
		}

		RUN_AT( SampleClock() + (int) getSampleRate(), &ug_filter_sv::OverflowCheck );
	}
}

// calculate resonance factor once only
float ug_filter_sv::CalcQ(float p_resonance)
{
	float q = p_resonance;
	q = max( q, 0.f );
	q = min( q, 0.9992f );// limit q to resonable values (under 9.992V)
	return 2.f - 2.f * q; // map to 0 - 2.0 range
}

float ug_filter_sv::CalcMSF(float p_resonance)
{
	float q = p_resonance;
	q = max( q, 0.f );
	q = min( q, 0.9992f );// limit q to resonable values (under 9.992V)
	return max_freq_lookup->GetValueFloatNoInterpolation( (int)(q * 512.f) ); // may need interpolation
}

float ug_filter_sv::CalcFreqFactor(float p_pitch)
{
	/*
		Looks like you want to update a filter for a double sampled state variable
		filter ;) Just use a cubic to approximate the sin since you only need to go
		up to pi/4 anyway.

		solve:

		x + c*x^3 == sin(x) for c

		c = (-x+sin(x))/x^3

		so at x=pi/4:

		c = -0.161601

		so you get:

		x - x*x*x*0.1616011013886495

	*/
	float result;

	// Within normal range use lookup table.
	if( p_pitch >= 0.0f && freq_scale == 0 )
	{
		// Note: this limits max frequency to 14080 Hz (EVEN WHEN OVERSAMPLING). See 1 Pole LP for improved version.
		//float index = min( p_pitch, 1.f );
		//result = fastInterpolation( index, TABLE_SIZE, m_lookup_table_data );
		result = fastInterpolationClipped( p_pitch, TABLE_SIZE, m_lookup_table_data );
	}
	else
	{
		float freq_hz;

		if( freq_scale == 0 ) // 1 Volt per Octave
		{
			freq_hz = VoltageToFrequency( p_pitch * 10.f );
		}
		else	// 1 Volt = 1 kHz
		{
			freq_hz = 10000.f * p_pitch;
		}

		result = PI2 * freq_hz / getSampleRate();

		if( result < 0.00001f ) // experimentally obtained
			result = 0.00001f;

		//	_RPT1(_CRT_WARN, "result %f\n", result );
	}

	// if either pitch or resonance change, limit frequency factor
	return min( result, max_stable_freq );
}

void ug_filter_sv::SetOutputState(timestamp_t p_clock, state_type p_state)
{
	OutputChange( p_clock, GetPlug(PN_LOW_PASS), p_state );
	OutputChange( p_clock, GetPlug(PN_HI_PASS), p_state );
	OutputChange( p_clock, GetPlug(PN_BAND_PASS), p_state );
	OutputChange( p_clock, GetPlug(PN_BAND_REJECT), p_state );
}
/*
> Biquads also do not need oversampling and they don't need the
> "stability criteria" in Andy's implementation (branch free coefficient
> calculation on per-sample base), that's one thing why they are
> computationally faster in my code.

Why not use a linear approximation to 2/f-f/2:
3.6f - 2.15f*f

You can also use a branch free min function (thanks Laurent from ohm force):
float minf(float a, float b)
{
	return 0.5f*(a+b - fabsf(a-b));
}

> The other is, for a biquad lowpass
> you need a sin() and a cos(), for the SVF you need a sin() and a pow().

You don't need pow. This is just just matches the damping with that of the
moog filter. Remember that damping=1/quality so if however you calculate the
'quality' factor for a biquad invert it to get your SVF damping.

Also remember that because you oversample the SVF you can get away with a
low order polynomial approximation of sin:

float sin3f(float x)
{
	return x - x*x*x/6.0f;
}

This works out to have an error of around 7 cents at nyquist.


*/
