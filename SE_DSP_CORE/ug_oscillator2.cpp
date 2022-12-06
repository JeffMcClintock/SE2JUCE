
/*
 FEATURE REQUESTS
-Is there a way to specify how SE oscillators would "sync" for different flavors of that Prophet 5 sync type lead? 
*/
#include "pch.h"
#include <math.h>
#include <algorithm>
#include <limits>
#include <iomanip>
#include <sstream>
#include "ug_oscillator2.h"

#include "ULookup.h"
#include "SeAudioMaster.h"
#include "module_register.h"
#include "./modules/shared/xplatform.h"

using namespace std;

// limit pulse width for P4 denormal probs at extremes
#define PW_MAX 0.99f

// define size of waveform tables
#define SINE_TABLE_SIZE 512
#define SAW_TABLE_SIZE 512
#define SYNC_CF_SAMPLES 4
// fading between wavetables
#define WAVE_CF_SAMPLES 20
#define FSampleToFrequency(volts) ( 440.f * powf(2.f, FSampleToVoltage(volts) - (float) MAX_VOLTS / 2.f ) )

// float version #define IncrementToFrequency(i) ( (double)(i) * (double)SampleRate() / (double)SINE_TABLE_SIZE )
#define IncrementToFrequency(i) ( (double)(i) * (double)SampleRate() / (1.0 + (double) UINT_MAX) )

// Pitch calculation.
static constexpr double octave_low = -10.0; // 0.0134 Hz
static constexpr double octave_hi  =  12.0; // 56 kHz
static constexpr double table_points_per_octave  =  12.0 * 4.0; // Table precise at semitone intervals.
static constexpr double table_size = ( octave_hi - octave_low ) * table_points_per_octave;


#define PITCH_TABLE_SIZE	512.f

#define PN_PITCH	0
#define PN_PW		1
#define PN_WAVEFORM	2
#define PN_OUTPUT	3
#define PN_SYNC		4
#define PN_PHASE_MOD	5
#define PN_FREQ_SCALE	6
#define PN_PM_DEPTH		7
#define PN_GIBBS_FIX	8

SE_DECLARE_INIT_STATIC_FILE(ug_oscillator2)

// create unnamed namespace to 'hide' stuff
namespace
{
REGISTER_MODULE_1( L"Oscillator", IDS_OSCILLATOR ,IDS_WAVEFORM, ug_oscillator2,CF_STRUCTURE_VIEW,L"The Oscillator is the starting point for many patches. It produces a choice of simple waveforms, plus white or pink noise.  The Phase Mod input is for Yamaha DX style 'frequency modulation' (works well with a sine-wave output).  See <a href=signals.htm>Signal Levels and Conversions</a> for Voltage to pitch conversion info");
}

// Fill an array of InterfaceObjects with plugs and parameters
void ug_oscillator2::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_PIN2( L"Pitch", pitch_ptr, DR_IN, L"5", L"100, 0, 10, 0", IO_POLYPHONIC_ACTIVE, L"1 Volt per Octave, 5V = Middle A");
	LIST_PIN2( L"Pulse Width", pulse_width_ptr, DR_IN, L"5", L"", IO_POLYPHONIC_ACTIVE, L"Width of rectangular pulse. -10 to 10 Volts = 0 to 100%");
	LIST_VAR3( L"Waveform",waveform, DR_IN,  DT_ENUM, L"1", L"Sine, Saw, Ramp, Triangle, Pulse,White Noise,Pink Noise", IO_POLYPHONIC_ACTIVE, L"Selects different wave shapes");
	LIST_PIN2( L"Audio Out", output_ptr, DR_OUT, L"0", L"", 0, L"");
	LIST_PIN2( L"Sync", sync_ptr, DR_IN, L"0", L"100, 0, 5, -5", IO_POLYPHONIC_ACTIVE, L"Syncs this Oscillator to an external signal, usually another oscillator to produce a gnarly sound.");
	LIST_PIN2( L"Phase Mod", phase_mod_ptr, DR_IN, L"0", L"100, 0, 10, 0", IO_POLYPHONIC_ACTIVE, L"Varies the phase (-5 to +5 Volts), used for yamaha style frequency modulation.  Don't work on Pulse waveform.");
	//	LIST_VAR3( L"Freq Scale", freq_scale,     DR _PARAMETER, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", 0, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
	LIST_VAR3( L"Freq Scale", freq_scale,     DR_IN, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", IO_MINIMISED, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
	LIST_PIN2( L"PM Depth", phase_mod_depth_ptr, DR_IN, L"5", L"100, 0, 10, 0", IO_POLYPHONIC_ACTIVE, L"Varies the phase modulation depth (0 to 10 Volts)");
	//	LIST_VAR3( L"Smooth Peaks (Gibbs Effect)", m_gibbs_fix, DR _PARAMETER, DT_BOOL, L"1", L"", 0, L"Improves the Wave Appearance by rolling off upper harmonics, reduces wave 'brightness' though.");
	//	LIST_VAR3( L"Sync X-Fade (Anti Alias)", m_smooth_sync, DR _PARAMETER, DT_BOOL, L"1", L"", 0, L"Reduces Alias noise when Syncing at Audio rates. Can be disabled to provide precise note-on phase sync (usefull for phase modulation patches)");
	LIST_VAR3( L"Smooth Peaks (Gibbs Effect)", m_gibbs_fix, DR_IN, DT_BOOL, L"1", L"", IO_MINIMISED, L"Improves the Wave Appearance by rolling off upper harmonics, reduces wave 'brightness' though.");
	LIST_VAR3( L"Sync X-Fade (Anti Alias)", m_smooth_sync, DR_IN, DT_BOOL, L"1", L"", IO_MINIMISED, L"Reduces Alias noise when Syncing at Audio rates. Can be disabled to provide precise note-on phase sync (usefull for phase modulation patches)");
	//TODO!!	LIST_VAR3( L"Anti Alias", m_anti_aliased, DR _PARAMETER, DT_BOOL, L"1", L"", 0, L"Essential for audio waves, but can be disabled on LFO's etc for more efficiency");
}

ug_oscillator2::ug_oscillator2() :
	cur_wave(NULL)
	,next_wave(NULL)
	,m_wave_data(NULL)
	,wavetable(NULL)
	,wave_fade_samples(0)
	,cur_phase(0)//-999) // allows easer check if inititaled b4 use
	,buf0(0)
	,buf1(0)
	,buf2(0)
	,buf3(0)
	,buf4(0)
	,buf5(0)
	,sync_flipflop(false)
	,count(0)
	,cross_fade_samples(0)
	,m_prev_sync_sample(0.f)
	,waveform(0) // pins no longer processed in order, gibs fix pin may be processed whiel waveform invalid (poly synth 2 in Cubase). (internal SDK)
{
}

int ug_oscillator2::Open()
{
	// Fill lookup tables if not done already
	InitPitchTable( this, freq_lookup_table );
	InitVoltToHzTable( this, lookup_table_volt_to_hz );
	
	// Fill sine lookup table if not done already
	if( sine_waves.empty() )
	{
		FillWaveTable( this, getSampleRate(), sine_waves, OWS2_SINE );
		//sine_waves[0].WaveTable()->min_inc = 0;
		//sine_waves[0].WaveTable()->min_band_inc = 0;

		// Fill silence lookup table if not done already.
		if( wave_overload.Create( this, L"Osc-Silence", -1 ) )
		{
			float* wave = wave_overload.WaveData();
			for( int j = 0 ; j < wave_overload.Size() ; j++ )
			{
				*wave++ = 0.f;
			}
		}

		wave_overload.WaveTable()->max_inc = UINT_MAX; // big number AMPLE;
		wave_overload.WaveTable()->min_inc = sine_waves[0].WaveTable()->max_inc; //highest_allowed_increment;
		wave_overload.WaveTable()->min_band_inc = wave_overload.WaveTable()->min_inc;
	}

	// Fill sawtooth lookup tables if not done already
	if( saw_waves.empty() || saw_waves[0].SampleRate() != getSampleRate() )
	{
		FillWaveTable( this, getSampleRate(), saw_waves, OWS2_SAW );
		FillWaveTable( this, getSampleRate(), saw_waves_no_gibbs, OWS2_SAW, false );
		// Fill triangle lookup tables if not done already
		FillWaveTable( this, getSampleRate(), tri_waves_no_gibbs, OWS2_TRI, false );
	}

	hz_to_inc_const = (UINT_MAX + 1.0f) / getSampleRate();

	random = Handle(); // provide randomness between oscs, but consistency on mutliple offline renders.

	ug_base::Open();

	RUN_AT( SampleClock(), &ug_oscillator2::OnFirstSample );
	//	_RPT2(_CRT_WARN, "Osc:  wave=%d buf=%x ", (int)waveform, plugs[PN_OUTPUT]->Get SamplePtr() );
/*
#if defined( _DEBUG )
	const float semitonev = 1.0f / 12.0f;
	for( float v = 0.3f ; v < 0.4f ; v += semitonev / 16.0f )
	{
		int i = calc_increment(v);
		float hz = FSampleToFrequency( v );
		int i2  = FrequencyToIntIncrement( SampleRate(), hz );
		_RPT3(_CRT_WARN, "%4f, %8d, %8d\n", v, i, i2 );
	}
#endif
	*/
	return 0;
}

// NOT BEING INLINED, when forced (__forceinline), made little difference
unsigned int ug_oscillator2::calc_increment(float p_pitch)// Re-calc increment for a new freq
{
	unsigned int increment;

	if( freq_scale == 0 ) // 1 Volt per Octave
	{
		/*
		#ifdef _DEBUG

			float volts = p_pitch * 10.f;
			int test_octave = floor(volts); // calls floor()
			float test_frac = volts - test_octave;
			float frac = (float)(*(unsigned int*)&dtemp)  / (float) 0x100000000;
			assert( test_frac >= 0.f && test_frac < 1.f );
			assert( fabs(test_octave+test_frac - (octave+frac)) < 0.001f );

			unsigned int tst_increment = freq_lookup_table->GetValueInter polated( frac );
			assert( abs( tst_increment - increment ) < 200 );
		#endif

		#ifdef _DEBUG

			float volts = p_pitch * 10.f;
			int test_octave = floor(volts); // calls floor()
			float test_frac = volts - test_octave;
			assert( test_frac >= 0.f && test_frac < 1.f );
			assert( fabs(test_octave+test_frac - (octave+frac)) < 0.001f );
		#endif
		*/
		//		increment = freq_lookup_table->GetValue Inter polated( frac );
		// max error with linear interpolation 0.000023 ( 0.0023 %)
		if( p_pitch > -1.f ) // ignore infinite or wild values, outside -20 -> +13 Volts (100kHz).
		{
#if 0
			// CPU 0.216% - SLOW!!!
			float freq_hz = FSampleToFrequency( p_pitch );
			//	freq_hz = min( freq_hz, SampleRate() / 2.f) ;
			increment = FrequencyToIntIncrement( getSampleRate(), freq_hz);
			//	_RPT3(_CRT_WARN, "%.2f V, %f Hz, inc = %d\n", p_pitch * 10.0f ,freq_hz , increment );
#else
			// CPU 0.068
			// Lookup Hz, then calc increment.
			float octavesFloat = p_pitch * 10.0f - octave_low;
			constexpr float c = table_size / (octave_hi-octave_low); // scale from volts to table index.
			float tableIdx = octavesFloat * c;

			int i = FastRealToIntTruncateTowardZero(tableIdx); // fast float-to-int using SSE. truncation toward zero.
			float idx_frac = tableIdx - (float) i;

			i = min( i, ((int)table_size) ); // not absolutely correct as we don't clamp fractional part, but fine in practice.

			const float* p = lookup_table_volt_to_hz->GetBlock() + i;
			float Hz = p[0] + idx_frac * (p[1] - p[0]);

			//increment = FrequencyToIntIncrement( SampleRate(), Hz); // speed up by removing division!!!!
			Hz = hz_to_inc_const * Hz + 0.5f;
			increment = FastRealToIntTruncateTowardZero(Hz);
#if 0
			// CPU 0.07%
			const int ensurePositive = 20;
			float octavesFloat = p_pitch * 10.0f + (float)ensurePositive; // 20 is to ensure result never negative, else truncation toward zero goes wrong way.

			// index = tableSize * max(index,0.0f);
			//index = ( index > 0.0f ) ? tableSize * index : 0.0f;

			// Split off octave.
			int octave = FastRealToIntTruncateTowardZero(&octavesFloat);

			// Fractional part withing octave used to lookup value.
			float tableIndex = 512.0f * ( octavesFloat - (float) octave );

			int i = FastRealToIntTruncateTowardZero(&tableIndex);
			float idx_frac = tableIndex - (float) i;

			const float* l_table = freq_lookup_table_float->table;

			const float* p = l_table + i;
			float inc = 0.5f + p[0] + idx_frac * (p[1] - p[0]);

			// Problem can only handle 31 bits, else overflows.
			increment = FastRealToIntTruncateTowardZero(&inc);

			int shift = octave - 8 - ensurePositive;
			if( shift < 0 )
			{
				increment = increment >> -shift;
			}
			else
			{
				int64_t result = increment << shift;
				if( result > 0x80000000 ) // & 0xffffffff00000000 )
				{ 
					// Result too big.
					increment = 0x80000000;
				}
				else
				{
					increment = result;
				}
			}

#endif
#endif
		}
		else
		{
			increment = 0;
		}

#ifdef _DEBUG

		// max error with linear interpolation 0.000023 ( 0.0023 %)
		if( p_pitch > -2.f && p_pitch < 1.2f ) // ignore infinite or wild values
		{
			float freq_hz = FSampleToFrequency( p_pitch );
			//	freq_hz = min( freq_hz, SampleRate() / 2.f) ;
			unsigned int correct_inc = FrequencyToIntIncrement( getSampleRate(), freq_hz);
			int error = abs( (int) (increment - correct_inc) );
			float percent = fabsf((float) error / (float)increment);
			//assert( abs(error) < 2 || percent < 0.00001f || increment == 0xffffffff ); // 0xffffffff indicates increment overflowed (pitch too high to hear)
			//_RPT3(_CRT_WARN, "%.2f V, %f Hz, inc = %d\n", p_pitch * 10.0f ,freq_hz , increment );
		}

#endif
	}
	else	// 1 Volt = 1 kHz
	{
		// not specificaly limited to nyquist here, just limited enough to prevent numeric overflow
		double freq_hz = min( 10000.0f * p_pitch, getSampleRate() / 2.f );
		freq_hz = max( freq_hz, 0.0 ); // only +ve freq allowed
		increment = FrequencyToIntIncrement( getSampleRate(), freq_hz);
		//	_RPT3(_CRT_WARN, "%f V, %f Hz, inc = %d\n", p_pitch * 10.0,freq_hz , increment );
	}

	return increment;
}

void ug_oscillator2::InitPitchTable( ug_base* p_ug, ULookup_Integer* &table)
{
	float sampleRate = p_ug->getSampleRate();
	p_ug->CreateSharedLookup2( L"osc pitch lookup", table, (int) sampleRate, PITCH_TABLE_SIZE+2, true, SLS_ALL_MODULES );

	//	_RPT1(_CRT_WARN, "InitPitchTable SR=%d\n", (int) p_ug->SampleRate() );
	if( !table->GetInitialised() )
	{
//		table->SetSize((int)PITCH_TABLE_SIZE+2);

		// store pitch over a one octave range 8 to 9 Volts
		// that's the highest increment that can be stored at 11kHz
		// other octaves are extracted by multiplying/dividing this table by powers of 2
		for( int j = 0 ; j < table->GetSize() ; j++ )
		{
			float pitch = 0.8f + 0.1f * (float) j / PITCH_TABLE_SIZE;
			float freq_hz = FSampleToFrequency( pitch );
			int inc = FrequencyToIntIncrement( sampleRate, freq_hz);
			table->SetValue( j, inc );
			//_RPT2(_CRT_WARN, "%3d : %6d\n",j, inc );
		}

		table->SetInitialised();
	}
}

void ug_oscillator2::InitPitchTableFloat( ug_base* p_ug, ULookup* &table)
{
	float sampleRate = p_ug->getSampleRate();
	p_ug->CreateSharedLookup2( L"osc pitch lookup float", table, (int) sampleRate, PITCH_TABLE_SIZE+2, true, SLS_ALL_MODULES );

	//	_RPT1(_CRT_WARN, "InitPitchTable SR=%d\n", (int) p_ug->SampleRate() );
	if( !table->GetInitialised() )
	{
//		table->SetSize((int)PITCH_TABLE_SIZE+2);

		// store pitch over a one octave range 8 to 9 Volts
		// that's the highest increment that can be stored at 11kHz
		// other octaves are extracted by multiplying/dividing this table by powers of 2
		for( int j = 0 ; j < table->GetSize() ; j++ )
		{
			float pitch = 0.8f + 0.1f * (float) j / PITCH_TABLE_SIZE;
			float freq_hz = FSampleToFrequency( pitch );
			float inc = (float) FrequencyToIntIncrement( sampleRate, freq_hz);
			table->SetValue( j, inc );
			//_RPT2(_CRT_WARN, "%3d : %f\n",j, inc );
		}

		table->SetInitialised();
	}
}

void ug_oscillator2::InitVoltToHzTable( ug_base* p_ug, ULookup* &table)
{
	float sampleRate = p_ug->getSampleRate();
	p_ug->CreateSharedLookup2( L"osc VtoHz", table, (int) sampleRate, (int)(table_size + 2.5 ), true, SLS_ALL_MODULES );

	if( !table->GetInitialised() )
	{
//		table->SetSize((int)(table_size + 2.5 ));

		const double safeMaximumHz = sampleRate * 0.55; // prevent wild aliasing (but allow a little).
		// store pitch over a one octave range 8 to 9 Volts
		// that's the highest increment that can be stored at 11kHz
		// other octaves are extracted by multiplying/dividing this table by powers of 2
		for( int j = 0 ; j < table->GetSize() ; j++ )
		{
			double volts =  octave_low + (octave_hi - octave_low) * (double) j / table_size;
			double freq_hz = 440.0 * pow( 2.0, volts - 5.0 );
			freq_hz = min( freq_hz, safeMaximumHz );
			table->SetValue( j, (float) freq_hz );
			//_RPT2(_CRT_WARN, "%3d : %f\n",j, inc );
		}

		table->SetInitialised();
	}
}

void ug_oscillator2::OnFirstSample()
{
	OutputChange( SampleClock(), GetPlug(PN_OUTPUT), ST_RUN );
	calc_phase( GetPlug(PN_PHASE_MOD)->getValue(), GetPlug(PN_PM_DEPTH)->getValue() );
	count = 0 - cur_phase;
	sync_flipflop = GetPlug(PN_SYNC)->getValue() <= 0.f;
}

void ug_oscillator2::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	// one_off change in phase
	if( p_to_plug == GetPlug(PN_PHASE_MOD) && p_state < ST_RUN )
	{
		count += calc_phase( GetPlug(PN_PHASE_MOD)->getValue(), GetPlug(PN_PM_DEPTH)->getValue() );
	}

	// one_off change in sync
	if( p_to_plug == GetPlug(PN_SYNC) && p_state < ST_RUN )
	{
		if( (GetPlug(PN_SYNC)->getValue() <= 0.f ) != sync_flipflop )
		{
			sync_flipflop = !sync_flipflop;

			if( !sync_flipflop ) // retrigger on -ve to +ve transition only
			{
				DoSync( GetPlug(PN_PHASE_MOD)->getValue(), GetPlug(PN_PM_DEPTH)->getValue(), 0 );
			}
		}
	}

	if( p_to_plug == GetPlug(PN_PITCH) || p_to_plug == GetPlug(PN_FREQ_SCALE) )
	{
		if( p_state < ST_RUN )
		{
			if( waveform < 5 ) // 5 and 6 are noise
			{
				fixed_increment = calc_increment( GetPlug(PN_PITCH)->getValue() );
				switch_wavetable( fixed_increment, getBlockPosition() );
			}

			/*
			#if !defined( SE_EDIT _SUPPORT )
						_RPT1(_CRT_WARN, " VST fixed_increment=%d\n", fixed_increment );
			#else
						_RPT1(_CRT_WARN, " SE  fixed_increment=%d\n", fixed_increment );
			#endif
						_RPT1(_CRT_WARN, "   Pitch=%f\n", GetPlug(PN_PITCH)->getValue() );
						_RPT1(_CRT_WARN, "   SampleRate()=%f\n", SampleRate() );
			*/
		}
	}

	if( p_to_plug == GetPlug(PN_WAVEFORM) ||  p_to_plug == GetPlug(PN_GIBBS_FIX) )//m_current_gibbs_fix != m_gibbs_fix)
	{
		//		_RPT1(_CRT_WARN, "   gibs=%d\n", m_gibbs_fix );
		ChangeWaveform( getBlockPosition() );
	}

	ChooseProcessFunction( getBlockPosition() );
}

void ug_oscillator2::ChooseProcessFunction( int blockPos )
{
	assert( blockPos >= 0 && blockPos < AudioMaster()->BlockSize() );

	switch( waveform )
	{
	case 5:
		SET_CUR_FUNC( &ug_oscillator2::process_white_noise );
		return;
		break;

	case 6:
		SET_CUR_FUNC( &ug_oscillator2::process_pink_noise );
		return;
		break;

	default:
		break;
	}

	// store pre-calculated pulse width if nesc
	if( waveform == 4 && GetPlug(PN_PW)->getState() != ST_RUN )
	{
		float pw = * ( GetPlug(PN_PW)->GetSamplePtr() + blockPos );
		pw = max( pw, -PW_MAX ); // reduce P4 denormals at extreme settings
		pw = min( pw, PW_MAX );
		pw = ( 1.f - pw ) * INT_MAX;
		fixed_pulse_width = (unsigned int) pw;
	}

	if( GetPlug(PN_PITCH)->getState() == ST_RUN )
	{
		if( GetPlug(PN_PHASE_MOD)->getState() == ST_RUN ) // pitch and phase
		{
			switch( waveform )
			{
			case 2:	// ramp
				SET_CUR_FUNC (&ug_oscillator2::sub_process_ramp);
				break;

			case 4: // pulse
				if( GetPlug(PN_PW)->getState() == ST_RUN )
				{
					SET_CUR_FUNC (&ug_oscillator2::sub_process_pw_pulse);
				}
				else
				{
					SET_CUR_FUNC (&ug_oscillator2::sub_process_pulse);
				}

				break;

			default:
				SET_CUR_FUNC (&ug_oscillator2::sub_process);
				break;
			}
		}
		else // pitch not phase
		{
			switch( waveform )
			{
			case 2:	// ramp
				SET_CUR_FUNC (&ug_oscillator2::sub_process_pitch_ramp);
				break;

			case 4: // pulse
				if( GetPlug(PN_PW)->getState() == ST_RUN )
				{
					SET_CUR_FUNC (&ug_oscillator2::sub_process_pw_pitch_pulse);
				}
				else
				{
					SET_CUR_FUNC (&ug_oscillator2::sub_process_pitch_pulse);
				}

				break;

			default:
				SET_CUR_FUNC (&ug_oscillator2::sub_process_pitch);
				break;
			}
		}
	}
	else // not pitch phase
	{
		if( GetPlug(PN_PHASE_MOD)->getState() == ST_RUN ) //&& (waveform < 2 || waveform == 3) )
		{
			switch( waveform )
			{
			case 2:	// ramp
				SET_CUR_FUNC (&ug_oscillator2::sub_process_phase_ramp);
				break;

			case 4: // pulse
				if( GetPlug(PN_PW)->getState() == ST_RUN )
				{
					SET_CUR_FUNC (&ug_oscillator2::sub_process_pw_phase_pulse);
				}
				else
				{
					SET_CUR_FUNC (&ug_oscillator2::sub_process_phase_pulse);
				}

				break;

			default:
				SET_CUR_FUNC (&ug_oscillator2::sub_process_phase);
				break;
			}
		}
		else // not pitch, not phase
		{
			switch( waveform )
			{
			case 2:	// ramp
				SET_CUR_FUNC (&ug_oscillator2::sub_process_fp_ramp);
				break;

			case 4: // pulse
				if( GetPlug(PN_PW)->getState() == ST_RUN )
				{
					SET_CUR_FUNC (&ug_oscillator2::sub_process_pw_fp_pulse);
				}
				else
				{
					SET_CUR_FUNC (&ug_oscillator2::sub_process_fp_pulse);
				}

				break;

			default:
				SET_CUR_FUNC (&ug_oscillator2::sub_process_fp);
				break;
			}
		}
	}

	// if nesc, run sync function, noting the function it replaces
	current_osc_func = current_osc_func2 = process_function;
	assert( waveform < 5 );

	if( GetPlug(PN_SYNC)->getState() == ST_RUN || cross_fade_samples > 0 )
	{
		SET_CUR_FUNC( &ug_oscillator2::process_with_sync );

		if( wave_fade_samples > 0 )
		{
			current_osc_func = static_cast <process_func_ptr> (&ug_oscillator2::process_wave_crossfade);
		}
	}
	else
	{
		if( wave_fade_samples > 0 )
		{
			SET_CUR_FUNC( &ug_oscillator2::process_wave_crossfade );
		}
	}
}

void ug_oscillator2::process_white_noise(int start_pos, int sampleframes)
{
	/* deliberate fail output buffer check
	if( start_pos > 0 )
	{
		start_pos = 0;
	}
	*/
	float* output = output_ptr + start_pos;
	unsigned int itemp;
	unsigned int idum = random;
	const unsigned int jflone = 0x3f800000; // see 'numerical recipies in c' pg 285
	const unsigned int jflmsk = 0x007fffff;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		idum = idum * 1664525L + 1013904223L;// use mask to quickly convert integer to float between -0.5 and 0.5
		itemp = jflone | (jflmsk & idum );
		*output++ = (*(float*)&itemp) -1.5f;
	}

	random = idum; // store new random number
}

void ug_oscillator2::process_pink_noise(int start_pos, int sampleframes)
{
	float* output = output_ptr + start_pos;
	unsigned int itemp;
	unsigned int idum = random;
	const unsigned int jflone = 0x3f800000; // see 'numerical recipies in c' pg 285
	const unsigned int jflmsk = 0x007fffff;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		idum = idum * 1664525L + 1013904223L;
		// use mask to quickly convert integer to float between -0.5 and 0.5
		itemp = jflone | (jflmsk & idum );
		float white = (*(float*)&itemp) -1.5f;
		buf0 = 0.997f * buf0 + 0.029591f * white;
		buf1 = 0.985f * buf1 + 0.032534f * white;
		buf2 = 0.950f * buf2 + 0.048056f * white;
		buf3 = 0.850f * buf3 + 0.090579f * white;
		buf4 = 0.620f * buf4 + 0.108990f * white;
		buf5 = 0.250f * buf5 + 0.255784f * white;
		*output++ = buf0 + buf1 + buf2 + buf3 + buf4 + buf5;
	}

	random = idum; // store new random number
}

void ug_oscillator2::process_wave_crossfade(int start_pos, int sampleframes)
{
	int sampleframes2 = min( sampleframes, wave_fade_samples );
	int save_count = count;
	//	float save_phase = cur _phase;
	unsigned int save_phase = cur_phase;
	(this->*(current_osc_func2))( start_pos, sampleframes2 );
	float* output = output_ptr + start_pos;
	// store the generated samples
	float store[WAVE_CF_SAMPLES];

	for( int i = 0 ; i < sampleframes2 ; i++ )
	{
		store[i] = *output++;
	}

	count = save_count;
	cur_phase = save_phase;
	OscWaveTable* cur_wave_store = cur_wave;
	float* m_wave_data_store = m_wave_data;
	cur_wave = next_wave;
	m_wave_data = next_wave->wave;
	(this->*(current_osc_func2))( start_pos, sampleframes2 );
	// mix in fadeout signal
	float fade = 1.0f - (20 - wave_fade_samples) / (float)(WAVE_CF_SAMPLES + 1);
	output = output_ptr + start_pos;

	for( int i = 0 ; i < sampleframes2 ; i++ )
	{
		fade -= 1.f / (WAVE_CF_SAMPLES + 1);
		*output = *output * ( 1.f - fade ) + store[i] * fade;
		output++;
	}

	wave_fade_samples -= sampleframes2;
	int remain = sampleframes - sampleframes2;

	if( remain > 0 )
	{
		(this->*(current_osc_func2))( start_pos + sampleframes2, remain );
	}

	if( wave_fade_samples == 0 )
	{
		// fade done
		int blockPos = start_pos + sampleframes - 1;
		float pitch = *(pitch_ptr + blockPos);
		switch_wavetable( calc_increment(pitch), blockPos ); // in case pitch has changed dramaticaly during x-fade
		ChooseProcessFunction( blockPos );
	}
	else // restore wave ptrs
	{
		cur_wave = cur_wave_store;
		m_wave_data = m_wave_data_store;
	}
}

void ug_oscillator2::process_sync_crossfade(int start_pos, int sampleframes)
{
	int sampleframes2 = min( sampleframes, cross_fade_samples );
	int save_count = count;
	unsigned int save_phase = cur_phase;
	count += sync_count_offset;
	(this->*(current_osc_func))( start_pos, sampleframes2 );
	float* output = output_ptr + start_pos;
	// store the generated samples
	float store[SYNC_CF_SAMPLES];

	for( int i = 0 ; i < sampleframes2 ; i++ )
	{
		store[i] = *output++;
	}

	count = save_count;
	cur_phase = save_phase;
	(this->*(current_osc_func))( start_pos, sampleframes2 );
	// mix in fadeout signal
	float fade = (float)cross_fade_samples / (float) SYNC_CF_SAMPLES;
	output = output_ptr + start_pos;
	int i;

	for( i = 0 ; i < sampleframes2 ; i++ )
	{
		fade -= 1.f / ( SYNC_CF_SAMPLES + 1 );
		*output = *output * ( 1.f - fade ) + store[i] * fade;
		output++;
	}

	cross_fade_samples -= sampleframes2;
	// process remainer after fade
	sampleframes -= sampleframes2;

	if( sampleframes > 0 )
	{
		start_pos += sampleframes2;
		(this->*(current_osc_func))( start_pos, sampleframes );
	}
}

void ug_oscillator2::DoSync( float p_phase, float p_phase_mod_depth, unsigned int p_zero_cross_offset )
{
	// reset counter
	unsigned int orig_count = count;
	// incorporate phase modulation
	calc_phase( p_phase, p_phase_mod_depth );
	count = 0 - cur_phase + p_zero_cross_offset;
	// calc offset between fade-out and fade-in counter
	sync_count_offset = orig_count - count;

	// set up cross fade
	if( m_smooth_sync )
	{
		cross_fade_samples = SYNC_CF_SAMPLES;
	}
}

void ug_oscillator2::process_with_sync(int start_pos, int sampleframes)
{
	float* sync	= sync_ptr + start_pos;
	// duration till next sync?
	int to_pos = start_pos;
	int last = start_pos + sampleframes;

	do
	{
		// to enable gate signal to trigger sync <= 0 is the test
		while( (*sync <= 0.f ) == sync_flipflop && to_pos < last )
		{
			to_pos++;
			sync++;
		}

		int sub_count = to_pos - start_pos;

		if( cross_fade_samples > 0 ) // sync cross fade in progress?
		{
			process_sync_crossfade( start_pos, sub_count );
		}
		else
		{
			(this->*(current_osc_func))( start_pos, sub_count );
		}

		// no. don't work if sync changes on last sample		if( to_pos == last ) // done?
		//no neither		if( (*sync <= 0.f ) == sync_flipflop && to_pos == last )
		if( to_pos == last )
		{
			// if sync was triggered by a one-off event, and cross fade is complete,
			// no further need for this function
			if( cross_fade_samples == 0 && GetPlug(PN_SYNC)->getState() == ST_STOP )
			{
				ChooseProcessFunction( last - 1 );
			}

			// store the last input sync value, in case needed next block
			m_prev_sync_sample = *(sync-1);
			//			assert(test == m_prev_sync_sample);
			return;
		}

		// do sync
		start_pos += sub_count;
		sync_flipflop = !sync_flipflop;

		if( !sync_flipflop ) // retrigger on -ve to +ve transition only
		{
			// calc approx time sync input crossed zero (linier interp)
			float y1;

			if( start_pos > 0 )
				y1 = *(sync-1);
			else
				y1 = m_prev_sync_sample; // need last sync input of previous block

			float y2 = *sync;
			float sub_sample_zero_cross = y2/(y2-y1);
			unsigned int zero_cross_offset = sub_sample_zero_cross * calc_increment(*(pitch_ptr + start_pos));
			float* phase_mod		= phase_mod_ptr + start_pos;
			float* phase_mod_depth	= phase_mod_depth_ptr + start_pos;
			DoSync( *phase_mod, *phase_mod_depth, zero_cross_offset );
		}
	}
	while(true);
}

// look up sample with interpolation
float ug_oscillator2::get_sample( float* wavedata, const unsigned int p_count) const
{
	// method 2: int counter. 16% faster on benchmark!
	const unsigned int jflone = 0x3f800000; // see 'numerical recipies in c' pg 285
	const unsigned int jflmsk = 0x007fffff;
	unsigned int itemp = jflone | (jflmsk & p_count );	// use low 23 bits as interpolation fraction
	float* p = wavedata + (p_count >> 23); // keep top 9 bits as index into 512 entry wavetable
	return p[0] + (p[1] - p[0]) * ((*(float*)&itemp) - 1.f);
	/*
	assert(fabs(result-test_result) < 0.0001f );
	return test_result;
	*/
	/* can't beat compiler
	#ifdef _DEBUG
			float result;
	#endif
			float idx_frac;
		__as m
		{
			mov		eax, dword ptr count  // index in eax
			mov		edx,eax
			and		eax, 0x007fffff
			shr		edx, 23						// int part in edx (table index)
			or		eax, 0x3f800000		// covert to float representation (+1.0)
			fld1
			mov     dword ptr [idx_frac], eax

			mov	  ecx, dword ptr[wavedata]
			fsubr  dword ptr [idx_frac]
	//		lea   esi,[ecx+edx*4]		// esi = & (table[int_idx] )

			fld   dword ptr [ecx+edx*4]       // * s1, [1.f - idx_frac}
			fld	  st(0)
			fsubr dword ptr [ecx+edx*4+4]       // * s1, [1.f - idx_frac}
			fmulp  st(2),st(0)         // * s1, [1.f - idx_frac}
			fadd
	#ifdef _DEBUG
			fstp   dword ptr result;
	#endif
		}
		*/
	/*
	// 	unsigned int itemp = jflone | (jflmsk & count );	// use low 23 bits as interpolation fraction

		mov	eax, DWORD PTR _count$[esp+4]

	// 	idx_frac = (*(float*)&itemp) - 1.f;			// high speed int->float conversion
	// 	float *p = wavedata + (count >> 23); // keep top 9 bits as index into 512 entry wavetable

		mov	edx, DWORD PTR _wavedata$[esp+4]
		mov	ecx, eax
		and	ecx, 8388607				; 007fffffH
		or	ecx, 1065353216				; 3f800000H
		mov	DWORD PTR _itemp$[esp+8], ecx
		fld	DWORD PTR _itemp$[esp+8]
		fsub	DWORD PTR __real@3f800000
		shr	eax, 23					; 00000017H

	// 	float s 1 = *p++;

		fld	DWORD PTR [edx+eax*4]
		lea	eax, DWORD PTR [edx+eax*4]

	//	float test_result = s1 + (*p - s1) * idx_frac;

		fld	DWORD PTR [eax+4]
		pop	esi
		fsub	ST(0), ST(1)
		fmulp	ST(2), ST(0)
		fxch	ST(1)
		fadd	ST(0), ST(1)
		fxch	ST(1)

	// 	return test_result;

		fstp	ST(0)
	*/
}

// convert phase change into increment delta
unsigned int ug_oscillator2::calc_phase( float phase, float modulation_depth)
{
	// !!!!	I've noticed that the SE_Osc phase is working backwards:-
	// confirmed (fix at some point).

	// integrate phase change and add to counter
	// convert phase to 32 bit counter offset
	// Get fractional part of phase.
	float a =  phase * modulation_depth;
	int b = FastRealToIntTruncateTowardZero(a);

	// Multiply by UINT_MAX. (note float-int conversion can't handle unsigned int, so mutiply by INT_MAX then after conversion by 2).
	float new_phase_count2 = (a - (float) b) * INT_MAX;
	unsigned int c = FastRealToIntTruncateTowardZero(new_phase_count2) << 1;
//	assert( abs((int) (c - count_delta[0])) < 3 );

	// calculate change in counter offset
	unsigned int phase_delta = cur_phase - c;
	cur_phase = c;
	return phase_delta;
}

// do-all sub-process. pitch, phase mod active
void ug_oscillator2::sub_process(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output			= output_ptr + start_pos;
	float* pitch			= pitch_ptr + start_pos;
	float* phase_mod		= phase_mod_ptr + start_pos;
	float* phase_mod_depth	= phase_mod_depth_ptr + start_pos;
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;
	switch_wavetable( calc_increment( *pitch ), start_pos );

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = get_sample( l_wave_data, c);
		c += calc_phase( *phase_mod++, *phase_mod_depth++ ) + calc_increment( *pitch++ );
	}

	count = c;
}

// sub-process. pitch active
void ug_oscillator2::sub_process_pitch(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output			= output_ptr + start_pos;
	float* pitch			= pitch_ptr + start_pos;
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;
	switch_wavetable( calc_increment( *pitch ), start_pos );

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = get_sample( l_wave_data, c);
		c += calc_increment( *pitch++ );
	}

	count = c;
}

// do-all sub-process. phase mod active
void ug_oscillator2::sub_process_phase(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output			= output_ptr + start_pos;
	float* phase_mod		= phase_mod_ptr + start_pos;
	float* phase_mod_depth	= phase_mod_depth_ptr + start_pos;
	const unsigned int l_increment = fixed_increment;
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = get_sample( l_wave_data, c);
		c += calc_phase( *phase_mod++, *phase_mod_depth++ ) + l_increment;
	}

	count = c;
}

// sub-process, fixed pitch, no phase modulation
void ug_oscillator2::sub_process_fp(int start_pos, int sampleframes)
{
	float* output = output_ptr + start_pos;
	assert( cur_wave != NULL );
	unsigned int c = count; // move to local var
	const unsigned int l_increment = fixed_increment;
	float* l_wave_data = m_wave_data;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = get_sample( l_wave_data, c);
		c += l_increment;
	}

	count = c;
	/*
		int test = 0;
		if( test != 0 )
		{
		for(int j = 0 ; j < 512 ; j = j + 5 )
		{
			for(int k = l_wave_data[j] * 40 + 20; k > 0 ; k-- )
			{
		_RPT0(_CRT_WARN, " " );
			}
		_RPT0(_CRT_WARN, "*\n" );
	//	_RPT1(_CRT_WARN, "%f\n", l_wave_data[j] );
		}
		}*/
}

///////////////RAMP //////////////////////

// fixed pitch, no phase modulation
void ug_oscillator2::sub_process_fp_ramp(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output	= output_ptr + start_pos;
//	float* pitch	= pitch_ptr + start_pos;
	unsigned int c = count; // move to local var
	const unsigned int l_increment = fixed_increment;
	float* l_wave_data = m_wave_data;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = -get_sample( l_wave_data, c);
		c += l_increment;
	}

	count = c;
}

// sub-process. pitch active
void ug_oscillator2::sub_process_pitch_ramp(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output			= output_ptr + start_pos;
	float* pitch			= pitch_ptr + start_pos;
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;
	switch_wavetable( calc_increment( *pitch ), start_pos );

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = -get_sample( l_wave_data, c);
		c += calc_increment( *pitch++ );
	}

	count = c;
}

// phase mod active
void ug_oscillator2::sub_process_phase_ramp(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output			= output_ptr + start_pos;
	float* phase_mod		= phase_mod_ptr + start_pos;
	float* phase_mod_depth	= phase_mod_depth_ptr + start_pos;
	const unsigned int l_increment = fixed_increment;
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = -get_sample( l_wave_data, c);
		c += calc_phase( *phase_mod++, *phase_mod_depth++ ) + l_increment;
	}

	count = c;
}

// same as sub_process, but inverts output
void ug_oscillator2::sub_process_ramp(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output			= output_ptr + start_pos;
	float* pitch			= pitch_ptr + start_pos;
	float* phase_mod		= phase_mod_ptr + start_pos;
	float* phase_mod_depth	= phase_mod_depth_ptr + start_pos;
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;
	switch_wavetable( calc_increment( *pitch ), start_pos );

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = -get_sample( l_wave_data, c);
		c += calc_phase( *phase_mod++, *phase_mod_depth++ ) + calc_increment( *pitch++ );
	}

	count = c;
}

const unsigned int jflone = 0x3f800000; // see 'numerical recipies in c' pg 285
const unsigned int jflmsk = 0x007fffff;

float ug_oscillator2::pulse_get_sample( float* wavedata, const unsigned int p_count, const unsigned int pulse_width) const
{
	unsigned int itemp = jflone | (jflmsk & p_count );	// use low 23 bits as interpolation fraction
	float* p = wavedata + (p_count >> 23); // keep top 9 bits as index into 512 entry wavetable
	float out = p[0] + (p[1] - p[0]) * ((*(float*)&itemp) - 1.f);

	// read offset ramp signal and combine to form pulse wave
	unsigned int c2 = p_count - pulse_width;	// unsigned 'wraps' value in correct range
	itemp = jflone | (jflmsk & c2 );			// use low 23 bits as interpolation fraction
	float* p2 = wavedata + (c2 >> 23);

	return (p2[0] + (p2[1] - p2[0]) * ((*(float*)&itemp) - 1.f)) - out;
}

// Same as above but need to calculate pulse width offset.
float ug_oscillator2::pulse_get_sample2( float* wavedata, const unsigned int p_count, float pulseWidth) const
{
#if 0
	pulseWidth = max( pulseWidth, -PW_MAX ); // reduce P4 denormals at extreme settings
	pulseWidth = min( pulseWidth, PW_MAX );
	pulseWidth = ( 1.f - pulseWidth ) * INT_MAX;

	int pulse_width = (int) pulseWidth;

	unsigned int itemp = jflone | (jflmsk & p_count );	// use low 23 bits as interpolation fraction
	float* p = wavedata + (p_count >> 23); // keep top 9 bits as index into 512 entry wavetable
	float out = p[0] + (p[1] - p[0]) * ((*(float*)&itemp) - 1.f);

	// read offset ramp signal and combine to form pulse wave
	unsigned int c2 = p_count - pulse_width - pulse_width;	// unsigned 'wraps' value in correct range
	itemp = jflone | (jflmsk & c2 );			// use low 23 bits as interpolation fraction
	p = wavedata + (c2 >> 23);

	return (p[0] + (p[1] - p[0]) * ((*(float*)&itemp) - 1.f)) - out;
#else
	pulseWidth = max( pulseWidth, -PW_MAX );
	pulseWidth = min( pulseWidth, PW_MAX );


	unsigned int itemp = jflone | (jflmsk & p_count );	// use low 23 bits as interpolation fraction
	float* p = wavedata + (p_count >> 23); // keep top 9 bits as index into 512 entry wavetable
	float out = p[0] + (p[1] - p[0]) * ((*(float*)&itemp) - 1.f);

	pulseWidth = ( 1.f - pulseWidth ) * (INT_MAX >> 1);
	int pulse_width = FastRealToIntTruncateTowardZero(pulseWidth);

	// read offset ramp signal and combine to form pulse wave
	unsigned int c2 = p_count - ( pulse_width << 1 );	// unsigned 'wraps' value in correct range
	itemp = jflone | (jflmsk & c2 );			// use low 23 bits as interpolation fraction
	float* p2 = wavedata + (c2 >> 23);

	return (p2[0] + (p2[1] - p2[0]) * ((*(float*)&itemp) - 1.f)) - out;

#endif
}

////////////////////PULSE WITHOUT PW ACTIVE /////////////////////

// sub-process fixed pitch, no phase modulation, no pulse modulation
void ug_oscillator2::sub_process_fp_pulse(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output = output_ptr + start_pos;
	unsigned int c = count; // move to local var
	const unsigned int l_increment = fixed_increment;
	const unsigned int l_fixed_pulse_width = fixed_pulse_width;
	float* l_wave_data = m_wave_data;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = pulse_get_sample( l_wave_data, c, l_fixed_pulse_width );
		c += l_increment;
	}

	count = c;
}

// uses mix of saw and ramp to produce variable pulse
void ug_oscillator2::sub_process_pulse(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output	= output_ptr + start_pos;
	float* pitch	= pitch_ptr + start_pos;
	float* phase_mod		= phase_mod_ptr + start_pos;
	float* phase_mod_depth	= phase_mod_depth_ptr + start_pos;
	//const unsigned int l_fixed_pulse_width = fixed_pulse_width;
	switch_wavetable( calc_increment( *pitch ), start_pos );
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = pulse_get_sample( l_wave_data, c, fixed_pulse_width );
		c += calc_phase( *phase_mod++, *phase_mod_depth++ ) + calc_increment( *pitch++ );
	}

	count = c;
}

// sub-process. pitch active
void ug_oscillator2::sub_process_pitch_pulse(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output			= output_ptr + start_pos;
	float* pitch			= pitch_ptr + start_pos;
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;
	const unsigned int l_fixed_pulse_width = fixed_pulse_width;
	switch_wavetable( calc_increment( *pitch ), start_pos );

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = pulse_get_sample( l_wave_data, c, l_fixed_pulse_width);
		c += calc_increment( *pitch++ );
	}

	count = c;
}

// do-all sub-process. phase mod active
void ug_oscillator2::sub_process_phase_pulse(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output			= output_ptr + start_pos;
	float* phase_mod		= phase_mod_ptr + start_pos;
	float* phase_mod_depth	= phase_mod_depth_ptr + start_pos;
	const unsigned int l_increment = fixed_increment;
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;
	const unsigned int l_fixed_pulse_width = fixed_pulse_width;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = pulse_get_sample( l_wave_data, c, l_fixed_pulse_width);
		c += calc_phase( *phase_mod++, *phase_mod_depth++ ) + l_increment;
	}

	count = c;
}
////////////////////PULSE WITH PW ACTIVE /////////////////////

// sub-process fixed pitch, no phase modulation, no pulse modulation
void ug_oscillator2::sub_process_pw_fp_pulse(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );

	float* pw		= pulse_width_ptr + start_pos;
	float* output	= output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = pulse_get_sample2( m_wave_data, count, *pw++ );
		count += fixed_increment;
	}
}

// uses mix of saw and ramp to produce variable pulse
void ug_oscillator2::sub_process_pw_pulse(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output	= output_ptr + start_pos;
	float* pitch	= pitch_ptr + start_pos;
	float* phase_mod		= phase_mod_ptr + start_pos;
	float* phase_mod_depth	= phase_mod_depth_ptr + start_pos;
	float* pw		= pulse_width_ptr + start_pos;
	switch_wavetable( calc_increment( *pitch ), start_pos );
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = pulse_get_sample2( l_wave_data, c, *pw++ );
		c += calc_phase( *phase_mod++, *phase_mod_depth++ ) + calc_increment( *pitch++ );
	}

	count = c;
}
// sub-process. pitch active
void ug_oscillator2::sub_process_pw_pitch_pulse(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output			= output_ptr + start_pos;
	float* pitch			= pitch_ptr + start_pos;
	float* pw		= pulse_width_ptr + start_pos;
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;
//	const int l_fixed_pulse_width = fixed_pulse_width;
	switch_wavetable( calc_increment( *pitch ), start_pos );

	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = pulse_get_sample2( l_wave_data, c, *pw++ );
		c += calc_increment( *pitch++ );
	}

	count = c;
}

// do-all sub-process. phase mod active
void ug_oscillator2::sub_process_pw_phase_pulse(int start_pos, int sampleframes)
{
	assert( cur_wave != NULL );
	float* output			= output_ptr + start_pos;
	float* phase_mod		= phase_mod_ptr + start_pos;
	float* phase_mod_depth	= phase_mod_depth_ptr + start_pos;
	float* pw		= pulse_width_ptr + start_pos;
	const unsigned int l_increment = fixed_increment;
	unsigned int c = count; // move to local var
	float* l_wave_data = m_wave_data;

	//	const int l_fixed_pulse_width = fixed_pulse_width;
	for( int s = sampleframes ; s > 0 ; --s )
	{
		*output++ = pulse_get_sample2( l_wave_data, c, *pw++ );
		c += calc_phase( *phase_mod++, *phase_mod_depth++ ) + l_increment;
	}

	count = c;
}
///////////////////END OF PULSE //////////////////////////////

void ug_oscillator2::ChangeWaveform( unsigned int blockPos )
{
	/*
		if( cur_wave_func )
		{
	///		run_function( cur_wave_func, ST_STOP );
			cur_wave_func = NULL;
		}
		if( cur_wavetable_func )
		{
	///		EventAbort( cur_wavetable_func );
			cur_wavetable_func = NULL;
		}
	*/
	//	_RPT1(_CRT_WARN, "ug_oscillator2::ChangeWaveform()%d\n", waveform );
	assert( waveform >= 0 && waveform < 7 );
	wavetable = NULL;
	cur_wave = NULL;

	switch( waveform )
	{
	case 0:
		wavetable = &sine_waves;
		break;

	case 1:	// sawtooth
	case 2:	// ramp
	case 4: // pulse
		if( m_gibbs_fix )
			wavetable = &saw_waves;
		else
			wavetable = &saw_waves_no_gibbs;

		break;

	case 3:
		//		if( m_gibbs_fix )
		//			wavetable = &tri_waves;
		//		else
		wavetable = &tri_waves_no_gibbs;
		break;

	case 5:
	case 6:
		break;

	default:
		break;
	}

	increment_lo_band = increment_hi_band = -1; // FORCE RECALC OF WAVETABLE

	/*
		// default to pure waveform
		if( cur_wave_func )
		{
			run_function( cur_wave_func, ST_RUN );
		}*/
	//	wavetable_mode = false;
	if( wavetable != NULL )
	{
		cur_wave = next_wave = (*wavetable)[0].WaveTable();
		m_wave_data = cur_wave->wave;

		if( waveform < 6 ) // 5 and 6 are noise
		{
			fixed_increment = calc_increment( GetPlug(PN_PITCH)->getValue() );
		}
	}
	else
	{
		cur_wave = next_wave = sine_waves[0].WaveTable(); // hack to prevent crashs in release mode
		fixed_increment = 0;
	}

	switch_wavetable( fixed_increment, blockPos );
}

// if nesc...
void ug_oscillator2::switch_wavetable( unsigned int increment, unsigned int blockPos )
{
	if( wave_fade_samples > 0 ) // fade already inprogress
		return;

	// decide whether to generate 'pure' sawtooth
	// or read anti-aliased version from table
	// do we need to change waveform table
	//	int i;
	// need to investigate why this is happening (in release version only?) !!!
	if( increment_lo_band > increment || increment_hi_band < increment )
	{
		if( next_wave != NULL && wavetable != NULL ) // can happen intermittant just after waveform switched
		{
			assert( next_wave );
			// check whether freq has strayed outside allowable range
			//		if( next_wave.min_band_inc > increment || next_wave->max_inc < increment )
			//			{
			/*			if( increment < (*wavetable)[0].min_band_inc )
						{
							_RPT1(_CRT_WARN, "\n %f Hz Switching to direct waveform generation\n", IncrementToFrequency(increment));
			/// 					run_function_exchange( cur_wavetable_func, cur_wave_func );
			//					wavetable_mode = false;
						}
						else
			*/
			{
				int i = wavetable->size() - 1;

				// trying to play too high freq?
				if ( (*wavetable)[i].WaveTable()->max_inc < increment )
				{
					//					_RPT1(_CRT_WARN, "\n inc %f\n", increment );
					//					_RPT0(_CRT_WARN, "\n Switching oscilator to silence\n");
					next_wave = wave_overload.WaveTable();
				}
				else
				{
					// search for correct wavetable (note it is posible for inc to be lower than min_inc (but > min_band_inc)
					for(  ; i > 0 && (*wavetable)[i].WaveTable()->min_inc > increment ; i-- )
						;

					//					i = max(i,0); // prevent going off bottom of table
					assert(i >= 0 );
					next_wave = (*wavetable)[i].WaveTable();
					//_RPT4(_CRT_WARN, "\n %.1f Hz Switching to wavetable %d ( %.0f -> %.0f Hz )\n",IncrementToFrequency(increment),i,IncrementToFrequency((*wavetable)[i].min_inc),IncrementToFrequency((*wavetable)[i].max_inc) );
				}

				//				wave_crossfade = 1.f;
				wave_fade_samples = WAVE_CF_SAMPLES;
				ChooseProcessFunction(blockPos);
				// set freq limits for this wave
				increment_lo_band = next_wave->min_band_inc;
				increment_hi_band = next_wave->max_inc;
				/*
								if( output > 0 )
									run_function( WaitZeroCross1, ST_RUN );
								else
									run_function( WaitZeroCross2, ST_RUN );
				*/
			}
		}

		if( cur_wave )
		{
			m_wave_data = cur_wave->wave;
		}
	}
}
/*
// output sawtooth
void ug_oscillator2::sawtooth(int start_pos, int sampleframes) // Ouput Sawtooth wave
{
	assert( cur_wave != NULL );

	float *output		= output_ptr + start_pos;
	float *pitch	= pitch_ptr + start_pos;
	float *sync		= sync_ptr + start_pos;
	float *phase_mod		= phase_mod_ptr + start_pos;
	float *phase_mod_depth	= phase_mod_depth_ptr + start_pos;

	// method 2: int counter. 16% faster on benchmark!
	const unsigned int jflone = 0x3f800000; // see 'numerical recipies in c' pg 285
	const unsigned int jflmsk = 0x007fffff;

	unsigned int c = count; // move to local var
	float *l_wave_data = m_wave_data;

	switch_wavetable(calc_increment( *pitch ));

	for( int s = sampleframes ; s > 0 ; --s )
	{
		unsigned int itemp = jflone | (jflmsk & c ); // use low 23 bits as interpolation fraction
		float idx_frac = (*(float*)&itemp) - 1.f;		// high speed int->float conversion

		float out = idx_frac; // output = counter

		*output++ = out;

		// integrate phase change and add to counter
		float phase = *phase_mod++;
		assert( cur_ phase != -999.f); // ensure it's been initialised
		float change = *phase_mod_depth++ * INT_MAX * (cur _phase - phase );	// scale change to 32 bit

		c += (int) change;
		cur _phase = phase;

		c += calc_increment( *pitch++ );
	}

	count = c;
} */
/*
void ug_oscillator2::ramp() // Ouput ramp wave
{
//	output = LONG_TO_SAMPLE(count) >> 1;
//	count -= increment;
}

void ug_oscillator2::pulse()
{
//	if( LONG_TO_SAMPLE(count) < *duty_cycle )
//		output = duty_cycle_lo;
//	else
//		output = duty_cycle_hi;

//	count += increment;
}

void ug_oscillator2::triangle()
{
	int temp = count;
	if( temp < 0 )
		temp = -temp;
	temp -= MAX_LONG / 2;
	output = LONG_TO_SAMPLE(temp);

	count += increment;
}*/

// you can't playback a harmonic up to the nyquist rate,
// this sets the 'safty margin'. was 4, but now 2
#define SEMITONES_BELOW_NYQUIST 2

//TODO: at higher sample rates, is there any point generating table for freq over 20kHz? waste of mem?
void ug_oscillator2::FillWaveTable( ug_base* ug, float sampleRate, wavetable_array& wa, osc_waveform2 waveshape, bool p_gibbs_fix)
{
	const double one_semitone_less = 1.0 / exp( log(2.) * 1 / 12 );
	const double safety_factor =  1.0 / exp( log(2.) * SEMITONES_BELOW_NYQUIST / 12 ); // 4 semitones less
	double scale_factor = 0;
	const int max_partials = SINE_TABLE_SIZE / 2;
	double level[max_partials];			// level of each partial
	double level_windowed[max_partials];// level of each partial with gobbs fix window applied
	double harmonic[max_partials];		// relates to freq of each partial
	int total_partials = 0;					// number of partials used
	bool cosines = false; // sum cosines (instead of sines?)

	std::wostringstream oss;
	oss << L"SeOsc2Wave" << waveshape << L"H" << 0 << L"G" << (int) p_gibbs_fix;

	// Assume if one lookup initialised at this sample-rate, they all are.
	ULookup* temp;
	ug->CreateSharedLookup2( oss.str(), temp, sampleRate, -1, false, SLS_ALL_MODULES );

	// If lookup tables already exist at this sample rate. Just grab pointers.
	if( temp != 0 )
	{
		int i = 0;
		do
		{
			oss.str(L"");
			oss << L"SeOsc2Wave" << waveshape << L"H" << i << L"G" << (int) p_gibbs_fix;

			ULookup* temp;
			ug->CreateSharedLookup2( oss.str(), temp, sampleRate, 0, false, SLS_ALL_MODULES );

			// Fill lookup tables if not done already
			if( temp == 0 )
			{
				return;
			}
			wa.push_back(0);

			wa[i].Init( temp, -1 );

			++i;

		}while(true);
	}

	//	_RPT0(_CRT_WARN, "-----------------------\n" );
	switch( waveshape )
	{
	case OWS2_SAW:
		//		_RPT0(_CRT_WARN, "Init Saw\n" );
	{
		scale_factor = 0.64;

		for( int h = max_partials - 1 ; h >= 0 ; h-- )
		{
			level[h] = -1.0 / (h+1);
			harmonic[h] = h + 1;
		}

		total_partials = max_partials;
	}
	break;

	case OWS2_TRI:
		//		_RPT0(_CRT_WARN, "Init Tri\n" );
	{
		scale_factor = 0.81;
		//			cosines = true;
		int i;

		for( i = 0 ; i < max_partials / 2; i++ )
		{
			int h = i + i + 1; // odd harmonics only
			level[i] = 1.0 / ( h * h );

			if( i & 1 ) // every 2nd harmonic inverted
				level[i] = -level[i];

			harmonic[i] = h;
		}

		total_partials = i;//max_partials;
	}
	break;

	case OWS2_COSINE:
		cosines = true;

	case OWS2_SINE:
		//		_RPT0(_CRT_WARN, "Init Sin\n" );
		scale_factor = 1.0;
		total_partials = 1;
		level[0] = 1.0;
		harmonic[0] = 1.0; // fundamental only
		break;
            
        default:
            break;
	};

	// now adjust partial levels
	scale_factor *= 0.5f; // adjust for floats

	for( int h = total_partials - 1 ; h >= 0 ; h-- )
	{
		level[h] *= scale_factor;
	}

	// last wavetable upper freq = 1/2 sample rate
	//	wa.RemoveAll();
	wa.clear();
	const int table_size = SAW_TABLE_SIZE;
	const int nyquist_inc = table_size / 2;
	double precalc_const1 = PI2 / (double) table_size;
	const double min_wt_gap = exp( log(2.) * 6 / 12 ); //6 semitones per wavetable minimum
	double max_inc,min_inc;
	int partial, next_partial = 0;
	double next_min_harmonic;
	// Calculate how many wavetables we need.
	int wavetableCount = 0;

	do
	{
		partial = next_partial;
		next_min_harmonic = (double) harmonic[partial] * min_wt_gap;

		if( partial == total_partials - 1) // last wt covers exactly 1 Plugin_Wrapper (b4 switch to direct waveform);
		{
			next_partial++;
			min_inc = nyquist_inc / ( 2.0 * harmonic[partial] ); // last table covers 1 Plugin_Wrapper regardless
		}
		else
		{
			do
			{
				next_partial++;
			}
			while( harmonic[next_partial] < next_min_harmonic && next_partial < total_partials - 1 );

			next_min_harmonic = harmonic[next_partial];
			min_inc = nyquist_inc / harmonic[next_partial];
		}

		max_inc = nyquist_inc / harmonic[partial];
		++wavetableCount;
	}
	while( next_partial < total_partials );

	// pre-allocate wavetables.
	wa.assign( wavetableCount, 0 );
	next_partial = 0;
	int currentTable = wavetableCount -1;

	do
	{
		partial = next_partial;
		next_min_harmonic = (double) harmonic[partial] * min_wt_gap;

		if( partial == total_partials - 1) // last wt covers exactly 1 Plugin_Wrapper (b4 switch to direct waveform);
		{
			next_partial++;
			min_inc = nyquist_inc / ( 2.0 * harmonic[partial] ); // last table covers 1 Plugin_Wrapper regardless
		}
		else
		{
			do
			{
				next_partial++;
			}
			while( harmonic[next_partial] < next_min_harmonic && next_partial < total_partials - 1 );

			next_min_harmonic = harmonic[next_partial];
			min_inc = nyquist_inc / harmonic[next_partial];
		}

		max_inc = nyquist_inc / harmonic[partial];
		//		_RPT4(_CRT_WARN, "max harmonic %3d (partial %3d),  max_inc %.4f (%3d)\n", (int)harmonic[partial], (int)partial, max_inc, (int)max_inc );

		SharedOscWaveTable& w = wa[currentTable];

		oss.str(L"");
		oss << L"SeOsc2Wave" << waveshape << L"H" << currentTable << L"G" << (int) p_gibbs_fix;

		if( w.Create( ug, oss.str(), static_cast<int>(sampleRate) ) )
		{
			// shift table increment to allow for fixed point index into wavetable (21 fractional bits)
			double temp	 = 0x80000000 / harmonic[partial]; //max_inc * (double)0x3;//(((unsigned int)max_inc) << 23);
			//float vers		double temp	 = 0.5f * SINE_TABLE_SIZE / harmonic[partial]; //max_inc * (double)0x3;//(((unsigned int)max_inc) << 23);
			// lower increment slightly below nyquest to prevent beating effects
			temp		 *= safety_factor;
			w.WaveTable()->max_inc = (int) temp;
			// lower again to provide 'hysterisis band' between wavetables
			temp		 *= one_semitone_less;
			int min_band_inc = (int) temp;//FrequencyToIncrement( chf * one_semitone_less );// (1 semitone hysterysis)

			// set prev wave's minimum to this wave's maximm
			//		if( wa.size() > 1 )
			if( (currentTable + 1) < wa.size() )
			{
				// wa[1].min_inc = w.max_inc;
				wa[ currentTable + 1 ].WaveTable()->min_inc = w.WaveTable()->max_inc;
				// allow some overlap between this and next lowest
				// to prevent too much table switching. ( 1 semitone )
				//wa[1].min_band_inc = min_band_inc; //wa[1].min_inc - VoltageToSample( 1.0/12.0 );// (1 semitone hysterysis)
				wa[ currentTable + 1 ].WaveTable()->min_band_inc = min_band_inc; //wa[1].min_inc - VoltageToSample( 1.0/12.0 );// (1 semitone hysterysis)
			}

			//				_RPT2(_CRT_WARN, " %.0f Harmonics ( up to %.1f Hz )\n", harmonic, SampleToFrequency(w.max_inc));
			//		_RPT3(_CRT_WARN, "max harmonic %3d (partial %3d),  max_inc %.2f \n", (int)harmonic[partial], (int)partial, w.max_inc );
			// apply window to levels
			if( p_gibbs_fix )
			{
				for( int h = partial ; h >= 0 ; h -- )
				{
					// windowing function to reduce gibbs phenomena (hamming)
					float window = 0.54f + 0.46f * cosf( ((float)harmonic[h] - 1.f) * PI / (partial+1) );
//					_RPT3(_CRT_WARN, "Harm %3d lev %f Gibbs %f\n",(int)harmonic[h], level[h], window  );
					level_windowed[h] = level[h] * window;
				}
			}
			else // straight copy
			{
				for( int h = partial ; h >= 0 ; h -- )
				{
					level_windowed[h] = level[h];
				}
			}
//			_RPT0(_CRT_WARN, "\n"  );

			// sum harmonics
			float* wave = w.WaveData();
			for( int j = 0 ; j < w.Size() ; j++ )
			{
				double sum = 0;
				double frac = precalc_const1 * j;

				if( cosines )
				{
					for( int h = partial ; h >= 0 ; h -- )
					{
						sum += level_windowed[h] * cos( harmonic[h] * frac );
					}
				}
				else
				{
					for( int h = partial ; h >= 0 ; h -- )
					{
						sum += level_windowed[h] * sin( harmonic[h] * frac );
					}
				}

				// w.lookup_->SetValue( j,(float) sum );
				*wave++ = (float) sum;
			}
		}

		--currentTable;
	}
	while( next_partial < total_partials );

	// set last wavetable to cover all frequencies down to zero Hz
	wa[0].WaveTable()->min_inc = 0;
	wa[0].WaveTable()->min_band_inc = 0;
}


/*
> - How to set up Chebyshev stuff to generate a defined number of
> harmonics

Chebyshev polynomials are defined in [-1 ; 1] by the formula
Tn(x) = cos (n * acos (x))

If you feed them with a sinusoid :

* t in [0 ; pi]

Tn (cos (t)) = cos (n * acos (cos (t)))
			  = cos (n * t)

* and t in [-pi ; 0[

Tn (cos (t)) = cos (n * acos (cos (t)))
because cos(t) = cos (-t) :
Tn (cos (t)) = cos (n * acos (cos (-t)))
			  = cos (n * -t)
again :
Tn (cos (t)) = cos (n * t)

So basically, the frequency of the sinusoidal input signal
is multiplied by the order of the Chebypoly. Just add
polynomials of different orders to produce a set of
harmonics.

The recurrence relation giving these polynomials is :

T0 (x) = 1
T1 (x) = x
n >= 2 : Tn (x) = 2 * x * Tn-1(x) - Tn-2(x)

-- Laurent

*/