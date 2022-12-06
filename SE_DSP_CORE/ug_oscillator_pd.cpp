#include "pch.h"
#include <algorithm>
#include <math.h>
#include "ug_oscillator_pd.h"

#include "ULookup.h"
#include "ug_oscillator2.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_oscillator_pd)

using namespace std;

namespace
{
REGISTER_MODULE_1(L"Phase Dist Osc", IDS_PDOSCILLATOR,IDS_WAVEFORM,ug_oscillator_pd ,CF_STRUCTURE_VIEW,L"The Casio CZ Oscillator <a href=\"phase_dist.htm\">more</a>");
}

// define size of waveform tables
#define SINE_TABLE_SIZE 512
#define FSampleToFrequency(volts) ( 440.f * powf(2.f, FSampleToVoltage(volts) - (float) MAX_VOLTS / 2.f ) )

// float version #define IncrementToFrequency(i) ( (double)(i) * (double)sample_rate / (double)SINE_TABLE_SIZE )
#define IncrementToFrequency(i) ( (double)(i) * (double)SampleRate() / (1.0 + (double) UINT_MAX) )

#define PITCH_TABLE_SIZE	512.f
#define MODULATION_MAX 0.97f

#define PN_PITCH		0
#define PN_MOD			1
#define PN_OUTPUT		2
#define PN_WAVEFORM1	3
#define PN_WAVEFORM2	4
#define PN_FREQ_SCALE	5
/*
sawtooth, square, pulse, double sine, saw-pulse, resonance 1 (sawtooth), resonance 2 (triangle) and resonance 3 (trapezoid).
(Note if you use a resonant waveform for one voice, the other voice can only select from waves 1 to 5.
*/

// Fill an array of InterfaceObjects with plugs and parameters
void ug_oscillator_pd::ListInterface2(InterfaceObjectArray& PList)
{
	LIST_PIN2( L"Pitch", pitch_ptr, DR_IN, L"5", L"100, 0, 10, 0", IO_POLYPHONIC_ACTIVE, L"1 Volt per Octave, 5V = Middle A");
	LIST_PIN2( L"Modulation Depth", modulation_ptr, DR_IN, L"5", L"", IO_POLYPHONIC_ACTIVE, L"Controls Waveshape distortion. 0 to 10 Volts = 0 to 100%");
	LIST_PIN2( L"Audio Out", output_ptr, DR_OUT, L"0", L"", 0, L"");
	LIST_VAR3( L"Wave1",wave1, DR_IN,  DT_ENUM, L"0", L"Saw,Square,Pulse,Dbl Sine,Saw-Pulse,Reso1,Reso2,Reso3", IO_POLYPHONIC_ACTIVE, L"Selects different wave shapes");
	LIST_VAR3( L"Wave2",wave2, DR_IN,  DT_ENUM, L"0", L"None=-1,Saw,Square,Pulse,Dbl Sine,Saw-Pulse,Reso1,Reso2,Reso3", IO_POLYPHONIC_ACTIVE, L"Selects different wave shapes");
	//	LIST_VAR3( L"Freq Scale", freq_scale,     DR _PARAMETER, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", 0, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
	LIST_VAR3( L"Freq Scale", freq_scale,     DR_IN, DT_ENUM   , L"0", L"1 Volt/Octave, 1 Volt/kHz ", IO_MINIMISED, L"Switches between 1 Volt per Octave or 1 Volt per kHz response.");
}

ug_oscillator_pd::ug_oscillator_pd() :
	count(0)
	,dc_offset(0.f)
{
}

int ug_oscillator_pd::Open()
{
	// Fill lookup tables if not done already
	//	const double highest_allowed_divisor = 2.01; // just below nyquist freqency
	//	double highest_allowed_freq = SampleRate() / highest_allowed_divisor;
	//	float highest_allowed_pitch = VoltageToFSample(Frequency ToVoltage( highest_allowed_freq ));
	ug_oscillator2::InitPitchTable( this, freq_lookup_table );

	/*
		CreateShared Lookup( L"ug_oscillator_pd freq inc", freq_lookup_table, SampleRate(), true, SLS_ALL_MODULES );
		if( !freq_lookup_table->GetInitialised() )
		{
			freq_lookup_table->SetSize(PITCH_TABLE_SIZE + 2);

			for( int j = 0 ; j < freq_lookup_table->GetSize() ; j++ )
			{
				float pitch = (float) j / PITCH_TABLE_SIZE;
				float freq_hz = FSampleToFrequency( pitch );
				freq_hz = min( freq_hz, SampleRate() / 2.f) ;
				float inc = FrequencyToIntIncrement(freq_hz);
				freq_lookup_table->SetValue( j, inc );
			}
			freq_lookup_table->SetInitialised();
		}
	*/
	// Fill sine lookup table if not done already
	if( sine_waves.empty() )
	{
		ug_oscillator2::FillWaveTable( this, -1, sine_waves, OWS2_COSINE );
		sine_waves[0].WaveTable()->min_inc = 0;
		sine_waves[0].WaveTable()->min_band_inc = 0;
	}

	ug_base::Open();
	fixed_increment = 0;
	m_wave_data = sine_waves[0].WaveData();
	FillSegmentTable();

	//DC removal filter, 7 Hz
	m_dc_filter_const = 2.f * sinf( PI * 7.f / getSampleRate() );
	OutputChange( SampleClock(), GetPlug(PN_OUTPUT), ST_RUN );
	return 0;
}

void ug_oscillator_pd::onSetPin(timestamp_t /*p_clock*/, UPlug* p_to_plug, state_type p_state)
{
	if( p_to_plug == GetPlug(PN_PITCH) || p_to_plug == GetPlug(PN_FREQ_SCALE) )
	{
		if( p_state < ST_RUN )
		{
			fixed_increment = calc_increment( GetPlug(PN_PITCH)->getValue() );
		}
	}

	if( p_to_plug == GetPlug(PN_WAVEFORM1) || p_to_plug == GetPlug(PN_WAVEFORM2) )
	{
		FillSegmentTable();
	}

	ChooseProcessFunction();
}

void ug_oscillator_pd::ChooseProcessFunction()
{
	if( m_freq_overload )
	{
		if( GetPlug(PN_PITCH)->getState() == ST_RUN )
		{
			SET_CUR_FUNC( &ug_oscillator_pd::sub_process_overload_pitch );
		}
		else
		{
			SET_CUR_FUNC( &ug_oscillator_pd::sub_process_overload );
		}
	}
	else
	{
		if( GetPlug(PN_PITCH)->getState() == ST_RUN )
		{
			SET_CUR_FUNC( &ug_oscillator_pd::sub_process );
		}
		else
		{
			SET_CUR_FUNC( &ug_oscillator_pd::sub_process_fp );
		}
	}
}

// do-all sub-process
void ug_oscillator_pd::sub_process(int start_pos, int sampleframes)
{
	float* output	= output_ptr + start_pos;
	float* mod		= modulation_ptr + start_pos;
	float* pitch	= pitch_ptr + start_pos;
	const unsigned int jflone = 0x3f800000; // see 'numerical recipies in c' pg 285
	const unsigned int jflmsk = 0x007fffff;
	unsigned int c = count; // move to local var
	unsigned int l_increment;
	float l_dc_offset = dc_offset;
	float* l_wave_data = m_wave_data;
	const float l_dc_filter_const = m_dc_filter_const;
	float x1,y1,x2,y2,amp = 1.0f;
	bool reso_mode = m_reso_section[m_cur_cycle];

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float modulation = *mod++;
		modulation = max( modulation, 0.f);
		modulation = min( modulation, MODULATION_MAX);
		//		float x = (float) c / (float) 0x100000000;
		int idum = c >> 9; //throw away 8 bits
		unsigned int itemp = jflone | (jflmsk & idum );
		float x = (*(float*)&itemp) -1.f;

		if( reso_mode )
		{
retry1:
			x2 = segments[m_segment+1].phase;

			if( x >= x2 )
			{
				m_segment++;
				goto retry1;
			}

retry1b:
			x1 = segments[m_segment].phase;

			if( x < x1 ) //less likely so last
			{
				m_segment--;
				x2 = x1;
				goto retry1b; // very important not to go recalc x2 (small numerical errors/rounding can cause endless looping)
			}

			y1 = segments[m_segment].mod_ammount;
			y2 = segments[m_segment+1].mod_ammount;
		}
		else
		{
retry2:
			x2 = segments[m_segment+1].phase + modulation * segments[m_segment+1].mod_ammount;

			if( x >= x2 )
			{
				m_segment++;
				goto retry2;
			}

retry2b:
			x1 = segments[m_segment  ].phase + modulation * segments[m_segment  ].mod_ammount;

			if( x < x1 )
			{
				m_segment--;
				x2 = x1;
				goto retry2b; // very important not to go recalc x2 (small numerical errors/rounding can cause endless looping)
			}

			y1 = segments[m_segment].phase2;
			y2 = segments[m_segment+1].phase2;
		}

		float slope = (y2-y1)/(x2-x1);
		float y = slope * (x - x1) + y1;

		if( reso_mode )
		{
			amp = y;
			y = x * ( 1.f + modulation * 10.f ); // reso 1
		}

		y *= 4294967296.f; // (float) 0x100000000;
		unsigned int dist_c = (unsigned int) y;
		itemp = jflone | (jflmsk & dist_c ); // use low 23 bits as interpolation fraction
		//		float idx_frac = (*(float*)&itemp) - 1.0f;		// high speed int->float conversion
		float* p = l_wave_data + (dist_c >> 23); // keep top 9 bits as index into 512 entry wavetable
		float out = p[0] + (p[1] - p[0]) * ((*(float*)&itemp) - 1.f);

		if( reso_mode )
		{
			out *= amp;
			out += 0.5f - amp * 0.5f;
		}

		*output++ = out - l_dc_offset;
		// !!!!could this go denormal after some time?, nuh. !!!!!!
		l_dc_offset -= (l_dc_offset-out) * l_dc_filter_const; // loose hi-pass filter
		l_increment = calc_increment( *pitch++ );

		if (c > UINT_MAX - l_increment) // phase wrapping?
		{
			m_cur_cycle++;
			m_segment = m_wrap_idx[m_cur_cycle];
			if( m_segment == 0 )
			{
				m_cur_cycle = 0;
			}
			reso_mode = m_reso_section[m_cur_cycle];
		}

		c += l_increment;
	}

	count = c;
	dc_offset = l_dc_offset;
}
/*
float dist( float phase, float modulation _depth )
{
//    phase = phase % (PI * 2);
	float PI = 0.5;
	float new_phase;
	float breakpoint = ( 1 - modulation _depth ) * PI / 2;

	// piecewise function in 3 linear sections
	if( phase < breakpoint )
	{
		new_phase = (PI / 2) * phase / breakpoint;
	}
	else
	{
		if ( phase < (2* PI - breakpoint ) )
		{
			new_phase = (phase-breakpoint)/(2*(PI-breakpoint)) * PI + PI/2;
		}
		else
		{
			new_phase = 2 * PI + (PI /2) * (phase - 2*PI ) / breakpoint;
		}
	}
	return new_phase - 0.25;
}
*/
// sub-process, fixed pitch, phase distortion
void ug_oscillator_pd::sub_process_fp(int start_pos, int sampleframes)
{
	float* output	= output_ptr + start_pos;
	float* mod		= modulation_ptr + start_pos;
	const unsigned int jflone = 0x3f800000; // see 'numerical recipies in c' pg 285
	const unsigned int jflmsk = 0x007fffff;
	unsigned int c = count; // move to local var
	unsigned int l_increment = fixed_increment;
	float l_dc_offset = dc_offset;
	float* l_wave_data = m_wave_data;
	const float l_dc_filter_const = m_dc_filter_const;
	float x1,y1,x2,y2,amp = 1.0f;
	bool reso_mode = m_reso_section[m_cur_cycle];

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		float modulation = *mod++;
		modulation = max( modulation, 0.f);
		modulation = min( modulation, MODULATION_MAX);
		//		float test_x = (float) c / (float) 0x100000000;
		// x must be >=0 and < 1.0 (never 1.0), causes wrong segment to be chosen
		// conert counter into a float between 0.0 and 1.0 (must never be 1.0 though)
		int idum = c >> 9; //throw away 8 bits
		unsigned int itemp = jflone | (jflmsk & idum );
		float x = (*(float*)&itemp) -1.f;
		assert( x >= 0.f && x < 1.f );

		if( reso_mode )
		{
			retry1:
			x2 = segments[m_segment+1].phase;

			if( x >= x2 )
			{
				m_segment++;
				goto retry1;
			}

			retry1b:
			x1 = segments[m_segment].phase;

			if( x < x1 ) //less likely so last
			{
				m_segment--;
				x2 = x1;
				goto retry1b;
			}

			y1 = segments[m_segment].mod_ammount;
			y2 = segments[m_segment+1].mod_ammount;
		}
		else
		{
			retry2:
			x2 = segments[m_segment+1].phase + modulation * segments[m_segment+1].mod_ammount;

			if( x >= x2 )
			{
				m_segment++;
				goto retry2;
			}

			retry2b:
			x1 = segments[m_segment  ].phase + modulation * segments[m_segment  ].mod_ammount;

			if( x < x1 )
			{
				m_segment--;
				x2 = x1;
				goto retry2b; // very important not to go recalc x2 (small numerical errors/rounding can cause endless looping)
			}

			y1 = segments[m_segment].phase2;
			y2 = segments[m_segment+1].phase2;
		}

		assert( x >= x1 && x <= x2 );
		float slope = (y2-y1)/(x2-x1);
		assert(x2 != x1); // check for division by zero
		float y = slope * (x - x1) + y1;

		if( reso_mode )
		{
			amp = y;
			y = x * ( 1.f + modulation * 10.f ); // reso 1
		}

		y *= 4294967296.f; // (float) 0x100000000;
		unsigned int dist_c = (unsigned int) y;
		itemp = jflone | (jflmsk & dist_c ); // use low 23 bits as interpolation fraction
		float* p = l_wave_data + (dist_c >> 23); // keep top 9 bits as index into 512 entry wavetable
		float out = p[0] + (p[1] - p[0]) * ((*(float*)&itemp) - 1.f);

		if( reso_mode )
		{
			out *= amp;
			out += 0.5f - amp * 0.5f;
		}

		// DC blocking stage
		*output++ = out - l_dc_offset;
		l_dc_offset -= (l_dc_offset-out) * l_dc_filter_const; // loose hi-pass filter
		//		c += l_increment;
		// done in assembler so i can access carry flag to detect counter wraparound easily
//		assert( false && "untested" );

//		unsigned int orig_count = c;
		//if( orig_count > c ) // wrapped?
//		if ((l_increment > 0) && (c > INT_MAX - l_increment)) // phase wrapping?
		if (c > UINT_MAX - l_increment) // phase wrapping?
		{
			m_cur_cycle++;
			m_segment = m_wrap_idx[m_cur_cycle];
			if( m_segment == 0 )
			{
				m_cur_cycle = 0;
			}
			reso_mode = m_reso_section[m_cur_cycle];
		}

		c += l_increment;
	}

	count = c;
	dc_offset = l_dc_offset;
}

// frequency has gone too high, output silence
void ug_oscillator_pd::sub_process_overload(int start_pos, int sampleframes)
{
	float* output = output_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*output++ = 0.f;
	}
}

void ug_oscillator_pd::sub_process_overload_pitch(int start_pos, int sampleframes)
{
	float* output = output_ptr + start_pos;
	// check if pitch has become legal again
	float* pitch  = pitch_ptr + start_pos;
	calc_increment(*pitch);

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*output++ = 0.f;
	}
}

void ug_oscillator_pd::FillSegmentTable()
{
	int i=0;
	m_segment = 0;
	m_cur_cycle = 0;
	short* wave = &wave1;
	int m_cycle_count = 0;

	while( true )
	{
		m_wrap_idx[m_cycle_count] = i;
		m_reso_section[m_cycle_count] = false; // does this section use the resonance algorythm?
		int j = i;

		switch( *wave )
		{
		case -1:	// None
				m_cycle_count--;
			break;

		case 0: // SAW
			{
				segments[i].phase		= 0.0;
				segments[i].mod_ammount	= 0.f;
				i++;
				segments[i].phase		= 0.5;
				segments[i].mod_ammount	= -0.5;
				i++;
				segments[i].phase		= 1.0;
				segments[i].mod_ammount	= 0.0;
				i++;
			}
			break;

		case 1:	// SQR
			{
				segments[i].phase		= 0.0;
				segments[i].mod_ammount	= 0.0;
				i++;
				segments[i].phase		= 0.0;
				segments[i].mod_ammount	= 0.25;
				i++;
				segments[i].phase		= 0.5;
				segments[i].mod_ammount	= -0.25;
				i++;
				segments[i].phase		= 0.5;
				segments[i].mod_ammount	= 0.25;
				i++;
				segments[i].phase		= 1.0;
				segments[i].mod_ammount	= -0.25;
				i++;
				segments[i].phase		= 1.0;
				segments[i].mod_ammount	= 0.0;
				i++;
				/* cool pulse wave
								segments[i].phase		= 0.0;
								segments[i].mod_ammount	= 0.0;
								i++;
								segments[i].phase		= 0.25;
								segments[i].mod_ammount	= -0.25;
								i++;
								segments[i].phase		= 0.25;
								segments[i].mod_ammount	= 0.25;
								i++;
								segments[i].phase		= 0.75;
								segments[i].mod_ammount	= -0.25;
								i++;
								segments[i].phase		= 0.75;
								segments[i].mod_ammount	= 0.25;
								i++;
								segments[i].phase		= 1.0;
								segments[i].mod_ammount	= 0.0;
								i++;
				*/
			}
			break;

		case 2: // CASIO PULSE
			{
				segments[i].phase		= 0.0;
				segments[i].mod_ammount	= 0.0;
				i++;
				segments[i].phase		= 0.0;
				segments[i].mod_ammount	= 0.5;
				i++;
				segments[i].phase		= 1.0;
				segments[i].mod_ammount	= -0.5;
				i++;
				segments[i].phase		= 1.0;
				segments[i].mod_ammount	= 0.0;
				i++;
			}
			break;

		case 4: // Half SINE
			{
				segments[i].phase		= 0.0;
				segments[i].mod_ammount	= 0.0;
				i++;
				segments[i].phase		= 0.5;
				segments[i].mod_ammount	= 0.0;
				i++;
				segments[i].phase		= 1.0;
				segments[i].mod_ammount	= -0.5;
				i++;
				segments[i].phase		= 1.0;
				segments[i].mod_ammount	= 0.0;
				i++;
			}
			break;

		case 3: // Double SINE
			{
				segments[i].phase		= 0.0;
				segments[i].phase2		= 0.0;
				segments[i].mod_ammount	= 0.0;
				i++;
				segments[i].phase		= 0.5;
				segments[i].phase2		= 1.0;
				segments[i].mod_ammount	= -0.5;
				i++;
				segments[i].phase		= 1.0;
				segments[i].phase2		= 2.0;
				segments[i].mod_ammount	= 0.0;
				i++;
				/*
								m_cycle_count++;
								m_reso_section[m_cycle_count] = false;
								m_wrap_idx[m_cycle_count] = i;

								segments[i].phase		= 0.0;
								segments[i].phase2		= 0.0;
								segments[i].mod_ammount	= -1.0;
								i++;
								segments[i].phase		= 1.0;
								segments[i].phase2		= 1.0;
								segments[i].mod_ammount	= 0.0;
								i++;
				*/
				/*

								segments[i].phase		= 0.0;
								segments[i].mod_ammount	= 0.0;
								i++;
								segments[i].phase		= 0.5;
								segments[i].mod_ammount	= 0.5;
								i++;
								segments[i].phase		= 1.0;
								segments[i].mod_ammount	= 0.0;
								i++;

								m_cycle_count++;
								m_reso_section[m_cycle_count] = false;
								m_wrap_idx[m_cycle_count] = i;

								segments[i].phase		= 0.0;
								segments[i].mod_ammount	= 0.0;
								i++;
								segments[i].phase		= 0.5;
								segments[i].mod_ammount	= -0.5;
								i++;
								segments[i].phase		= 1.0;
								segments[i].mod_ammount	= 0.0;
								i++;*/
			}
			break;

		case 5: // SAW RESO
			{
				m_reso_section[m_cycle_count] = true;
				segments[i].phase		= 0.0;
				segments[i].mod_ammount	= 1.0;
				i++;
				segments[i].phase		= 1.0;
				segments[i].mod_ammount	= 0.0;
				i++;
			}
			break;

		case 6: // TRI RESO
			{
				m_reso_section[m_cycle_count] = true;
				segments[i].phase		= 0.0;
				segments[i].mod_ammount	= 0.0;
				i++;
				segments[i].phase		= 0.5;
				segments[i].mod_ammount	= 1.0;
				i++;
				segments[i].phase		= 1.0;
				segments[i].mod_ammount	= 0.0;
				i++;
			}
			break;

		case 7: // TRAP RESO
			{
				m_reso_section[m_cycle_count] = true;
				segments[i].phase		= 0.0;
				segments[i].mod_ammount	= 1.0;
				i++;
				segments[i].phase		= 0.5;
				segments[i].mod_ammount	= 1.0;
				i++;
				segments[i].phase		= 1.0;
				segments[i].mod_ammount	= 0.0;
				i++;
			}
			break;
		};

		if( *wave != 3 ) // double sine is the weird one
		{
			while( j < i )
			{
				segments[j].phase2 = segments[j].phase;
				j++;
			}
		}

		m_cycle_count++;

		if( wave == &wave1 )
		{
			wave = &wave2;
		}
		else
		{
			assert( i < 19 );
			assert( m_cycle_count < 8 );
			m_wrap_idx[m_cycle_count] = 0; // end marker
			return;
		}
	}
}

unsigned int ug_oscillator_pd::calc_increment(float p_pitch) // Re-calc increment for a new freq
{
	unsigned int increment;

	if( freq_scale == 0 ) // 1 Volt per Octave
	{
		// max error with linear interpolation 0.000023 ( 0.0023 %)
		if( p_pitch > -11.f && p_pitch < 12.f ) // ignore infinite or wild values
		{
			float freq_hz = FSampleToFrequency( p_pitch );
			//	freq_hz = min( freq_hz, SampleRate() / 2.f) ;
			increment = FrequencyToIntIncrement( getSampleRate(), freq_hz);
			//	_RPT3(_CRT_WARN, "%.2f V, %f Hz, inc = %d\n", p_pitch * 10.0f ,freq_hz , increment );
		}
		else
		{
			increment = 0;
		}
	}
	else	// 1 Volt = 1 kHz
	{
		// not specificaly limited to nyquist here, just limited enough to prevent numeric overflow
		double freq_hz = min( 10000.0f * p_pitch, getSampleRate() / 2.f );
		freq_hz = max( freq_hz, 0.0 ); // only +ve freq allowed
		increment = FrequencyToIntIncrement(getSampleRate(), freq_hz);
		//			_RPT3(_CRT_WARN, "%f V, %f Hz, inc = %d\n", p_pitch * 10.0,freq_hz , increment );
	}

	//!!todo limit modulation based on freqency relative to nyquist
	// trying to play too high freq?
	bool freq_overload = sine_waves[0].WaveTable()->max_inc < increment;

	if( freq_overload != m_freq_overload )
	{
		m_freq_overload = freq_overload;
		ChooseProcessFunction();
	}

	return increment;
}