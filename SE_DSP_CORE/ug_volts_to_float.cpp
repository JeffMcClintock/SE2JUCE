#include "pch.h"
#include <math.h>
#include "ug_volts_to_float.h"
#include "conversion.h"
#include "module_register.h"
#include "UgDatabase.h"

SE_DECLARE_INIT_STATIC_FILE(ug_volts_to_float)

#define PN_INPUT		0
#define PN_MODE			1
#define PN_UPDATE_RATE	2
#define PN_OUTPUT		3

namespace
{
static pin_description plugs[] =
{
	L"Volts In", DR_IN, DT_FSAMPLE	,L"", L"None,Fast (4 samp),Smooth (30ms)", IO_LINEAR_INPUT, L"",
	L"Response"	, DR_IN, DT_ENUM	,L"4", L"dB VU,dB PPM,dB Peak,dB HeadRoom, Volts DC (Fast), Volts DC Average, Volts RMS, Clip Detect", 0, L"", // fix ui_param_float_in::Up grade() if renaming
	L"Update Rate", DR_IN, DT_ENUM,L"10", L"1 Hz=1,5 Hz=5,10 Hz=10,20 Hz=20,40 Hz=40,60 Hz=60", 0, L"", // fix ui_param_float_in::Up grade() if renaming
	L"Float Out", DR_OUT, DT_FLOAT	,L"", L"", 0, L"",
};

static module_description_dsp mod =
{
	"VoltsToFloat", IDS_VOLTS_TO_FLOAT, IDS_MG_OLD , &ug_volts_to_float::CreateObject, CF_STRUCTURE_VIEW,plugs, std::size(plugs)
};

bool res = ModuleFactory()->RegisterModule( mod );

static module_description_dsp mod2 =
{
	"VoltsToFloat2", IDS_VOLTS_TO_FLOAT2, IDS_MG_CONVERSION , &ug_volts_to_float2::CreateObject, CF_STRUCTURE_VIEW,plugs, std::size(plugs)
};

bool res2 = ModuleFactory()->RegisterModule( mod2 );
}

void ug_volts_to_float::assignPlugVariable(int p_plug_desc_id, UPlug* p_plug)
{
	switch( p_plug_desc_id )
	{
	case PN_INPUT:
		p_plug->AssignVariable( 0, &input_ptr );
		break;

	case PN_MODE:
		p_plug->AssignVariable( &mode );
		break;

	case PN_UPDATE_RATE:
		p_plug->AssignVariable( &m_update_hz );
		break;

	case PN_OUTPUT:
		p_plug->AssignVariable( &output_val );
		break;

	default:
		break;
	};
}

enum
{
	VU, PPM, Peak, Headroom, Fast_DC, Average_DC, AC_RMS, Clip_Detect
};


ug_volts_to_float::ug_volts_to_float() :
	monitor_routine(NULL)
	,max_input(0.f)
	,current_max(0.f)
	,monitor_routine_running (false)
	,mode(Peak)
	,m_threshold(0.5f)
	,m_env(0.f)
	,y1n(0.f)
{
	SetFlag(UGF_POLYPHONIC_AGREGATOR|UGF_VOICE_MON_IGNORE);
}

int ug_volts_to_float::Open()
{
	ug_base::Open();

	if( mode != 0 )
		threshold_ptr = NULL;

	mode_change();
	current_max = -1000.f; // force initial update
	// no, shouldn't access input yet	(this->*(monitor_routine))();
	RUN_AT( 0, monitor_routine );
	monitor_routine_running = true;
	// meter meant to respond in 20ms, that's 50Hz, set LP to 100 to be sure
	// Balistics set elswhere, this is just the RMS average filter
	const float cuttof_freq = 100; // hz
	low_pass_cof = exp( -PI2 * cuttof_freq / getSampleRate());
	return 0;
}

void ug_volts_to_float::sub_process_rms(int start_pos, int sampleframes)
{
	float* input = input_ptr + start_pos;

	//_RPT0(_CRT_WARN, "ug_volts_to_float::sub_process\n" );
	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float rectified = *input++;
		// squared
		rectified *= rectified;
		// low-pass filter (mean)
		y1n = rectified + low_pass_cof * ( y1n - rectified );

		float l = sqrtf(y1n); // (root) MSVC uses SSE so it's pretty fast without further optimisation. But could reduce rate at which it's calced via a counter, say 1kHz.

		float delta = m_env - l;

		if ( delta < 0.f )	//input's increasing
			delta *= m_attack;
		else
			delta *= m_release;

		//m_env = kill_denormal2(l + delta);
		m_env = l + delta;
	}
}

void ug_volts_to_float::sub_process_peak(int start_pos, int sampleframes)
{
	float* input = input_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float l = fabsf(*input++);
		float delta = m_env - l;

		if ( delta < 0.f )	//input's increasing
			delta *= m_attack;
		else
			delta *= m_release;

		//m_env = kill_denormal2(l + delta);
		m_env = (l + delta);
	}
}

void ug_volts_to_float::sub_process_clip_detect(int start_pos, int sampleframes)
{
	float* input = input_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		if( *input >= 1.0f || *input <= -1.0f )
		{
			m_env = 1.0f;
		}
		++input;
	}
}

void ug_volts_to_float::sub_process_dc(int start_pos, int sampleframes)
{
	float* input = input_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float l =*input++;
		float delta = (m_env - l) * m_release; // must have equal attack/release to get average DC Volts
		//m_env = kill_denormal2(l + delta);
		m_env = (l + delta);
	}
}

void ug_volts_to_float::onSetPin(timestamp_t /*p_clock*/, UPlug* p_to_plug, state_type p_state)
{
	assert(sleeping == false);

	if( p_to_plug == GetPlug(PN_MODE) )
	{
		mode_change();
	}

	if( p_to_plug == GetPlug(PN_UPDATE_RATE) )
	{
		// try to avoid strobing effects with test tones
		float t = m_update_hz + 0.03f;
		t = getSampleRate() / t;
		delay = (int) t;
	}

	if( p_to_plug == GetPlug(PN_INPUT) )
	{
		//		DEBUG_TIMESTAMP;
		//		_RPT2(_CRT_WARN, "ug_volts_to_float::on_input_stat_change STAT = %s, %.2f\n",  GetStatString(p_state), 10.0 * GetPlug(0)->getValue() );
		if( p_state > ST_STOP )
		{
			// avoid delay when in fast-mode and it's a simple one-off update
			if (mode == Fast_DC && p_state == ST_ONE_OFF)
			{
				output_val = 10.f * GetPlug(PN_INPUT)->getValue();
				SendPinsCurrentValue(SampleClock(), GetPlug(PN_OUTPUT));
			}
			else
			{
				SET_CUR_FUNC(audio_routine);

				if (!monitor_routine_running)
				{
					max_input = 0.f;
					monitor_routine_running = true;
					RUN_AT(SampleClock() + delay, monitor_routine);
				}

				monitor_done = false;
			}
		}
	}
}

void ug_volts_to_float::mode_change()
{
	//set ballistics
	switch( mode )
	{
	case AC_RMS:
	case Average_DC:
	case VU:		// standard VU has 300mS integration (100mS time const)
		m_attack = m_release = exp( -1.f / ( 0.1f * getSampleRate() ));
		break;

	case PPM:		// standard PPM has 5mS attack integration, 1.4-2s decay
		m_attack = exp( -1.f / ( 0.017f * getSampleRate() ));
		m_release = exp( -1.f / ( 0.650f * getSampleRate() ));
		break;

	case Peak:		// digital peak - no standard... instant attack, 100mS release int.
	case Headroom:	// headroom is digital peak - with output of difference from full scale
	default:
		m_attack = 0.f;
		m_release = exp( -1.f / ( 0.03f * getSampleRate() ));
		break;
	}

	//set averaging
	switch( mode )
	{
	case Average_DC:
		audio_routine = static_cast <process_func_ptr> (&ug_volts_to_float::sub_process_dc);
		break;

	case Fast_DC:
		audio_routine = static_cast <process_func_ptr> (&ug_base::process_nothing);
		break;

	case Headroom:	// headroom is digital peak - with output of difference from full scale
	case Peak:		// digital peak - no standard... instant attack, 100mS release int.
		audio_routine = static_cast <process_func_ptr> (&ug_volts_to_float::sub_process_peak);
		break;

	case Clip_Detect:		// Clip Detect.
		audio_routine = static_cast <process_func_ptr> (&ug_volts_to_float::sub_process_clip_detect);
		break;

	default:
		audio_routine = static_cast <process_func_ptr> (&ug_volts_to_float::sub_process_rms);
		break;
	}

	monitor_routine = static_cast <ug_func> (&ug_volts_to_float::monitor);
	SET_CUR_FUNC( audio_routine );
}

void ug_volts_to_float::monitor()
{
	//	DEBUG_TIMESTAMP;
	//	_RPT0(_CRT_WARN, "ug_volts_to_float::monitor()\n" );
	float output_level;

	/*
		if( mode == Headroom)
			output_level = 1.f - m_env;
		else
			output_level = m_env;

		if( mode == Fast_DC ) // just samples input every so often
		{
			output_level = GetPlug(PN_INPUT)->getValue();
		}
	*/
	if( !( m_env < 1000000.0f ) ) // fix overflow.
	{
		m_env = 0.0f;
	}

	if( !( y1n < 1000000.0f ) ) // fix overflow.
	{
		y1n = 0.0f;
	}

	if( mode == Fast_DC )
	{
		output_level = GetPlug(PN_INPUT)->getValue();
	}
	else
	{
		output_level = m_env;
	}

	float delta = fabsf(current_max - output_level);

	// prevent extended fade out
	if( delta < 0.00000001f )
	{
		delta = 0.f;
	}
	else  //if( delta > 0.f ) // need to update UI?
	{
		monitor_done = false;	// signal still decaying?, keep awake.
		current_max = output_level;

		if( mode < Fast_DC )
		{
			// minimum -160 dB
			if( current_max < 0.00000001 )
				output_val = -160.f;
			else
				output_val = 20.f * log10f(current_max); // Full-Scale dB (dBFS)

			// full VU meter spec (ANSI C165, also IEC60268-17 )
			// EBU Digital   says 0dBFS = 18 dBu
			// SMPTE Digital says 0dBFS = 24 dBu
			// XXXX Attempt compat with Cubase (0dBFS = 10 dBu)

			// RMS measures 3dB lower than peak (on a sine anyhow). Compensate so FS = 0dB.
			if( mode <= PPM )
			{
				output_val += 3.f;
			}

			if( mode == Headroom )
			{
				output_val = -output_val;
			}
		}
		else
		{
			output_val = current_max * 10.f;
		}

		SendPinsCurrentValue( SampleClock(), GetPlug(PN_OUTPUT) );
	}

	if( mode == Clip_Detect )
	{
		m_env = 0.0f;
	}

	if( monitor_done )
	{
		monitor_routine_running = false;
		SET_CUR_FUNC( &ug_base::process_sleep );
	}
	else
	{
		SET_CUR_FUNC( audio_routine ); // must be restarted
		RUN_AT( SampleClock() + delay, &ug_volts_to_float::monitor );
	}

	if( GetPlug(PN_INPUT)->getState() != ST_RUN )
	{
		monitor_done = true;	// make note to stop monitor routine AFTER next time
	}
}

/*
// SSE instructions for getting signal peak. not significantly faster, but could be improved further.

// process fiddly non-sse-aligned prequel.
while (sampleFrames > 0 && reinterpret_cast<intptr_t>(input) & 0x0f)
{
values[index] = (std::max)(values[index], abs(*input));
++input;
--sampleFrames;
}

// SSE intrinsics
__m128* pIn1 = (__m128*) input;
__m128 rMax = _mm_set_ps1(0.0f);
__m128 rMin = _mm_set_ps1(0.0f);

// move first runing input plus static sum.
while (sampleFrames > 0)
{
rMin = _mm_min_ps(rMin, *pIn1);
rMax = _mm_max_ps(rMax, *pIn1);

++pIn1;
sampleFrames -= 4;
}

// rMax = max( rMax, -rMin);
rMax = _mm_max_ps(rMax, _mm_xor_ps(rMin, _mm_set1_ps(-0.0f)));

float r[4];
_mm_storeu_ps(r, rMax);

values[index] = (std::max)(values[index], (std::max)(r[0], (std::max)(r[1], (std::max)(r[2], r[3]))) );

*/