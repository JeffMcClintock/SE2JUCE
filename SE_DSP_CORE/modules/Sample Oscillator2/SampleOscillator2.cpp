#include <math.h>
#include "./SampleOscillator2.h"
#include "./csoundfont.h"
#include "./SampleManager.h"

#define INTERPOLATION_POINTS 8
#define INTERPOLATION_DIV 32

const int tableSize_ = 2 * ( INTERPOLATION_POINTS / 2 + 1 ) * INTERPOLATION_DIV;
static float interpolationTable[ tableSize_ ];
static bool interpolationTableInitialized = false;
float* SincInterpolator::interpolation_table2 = 0;

REGISTER_PLUGIN ( SoundfontOscillator2, L"SE Sample Oscillator2" );

/* notes
Soundfonts don't unload until all voices have been woken once, can take sometime during which both soundfonts
consume memory, same when changing patch (each patch currently treated as new soundfont).
*/

SoundfontOscillator2::SoundfontOscillator2( IMpUnknown* host ) : MpBase( host )
,trigger_state( false )
,gate_state( false )
{
	// Register pins.
	initializePin( 0, pinSampleId );
	initializePin( 1, pinPitch );
	initializePin( 2, pinTrigger );
	initializePin( 3, pinGate );
	initializePin( 4, pinVelocity );
	initializePin( 5, pinQuality );
	initializePin( 6, pinLeft );
	initializePin( 7, pinRight );

	SET_PROCESS( &SoundfontOscillator2::sub_process_silence );
}

void SoundfontOscillator2::onGraphStart()
{
	MpBase::onGraphStart();

	pinLeft.setStreaming( false );
	pinRight.setStreaming( false );
}

void SoundfontOscillator2::onSetPins()
{
	if( pinGate.isUpdated() || pinTrigger.isUpdated() )
	{
		if(	pinGate.isStreaming() || pinTrigger.isStreaming() )
		{
			SET_PROCESS( &SoundfontOscillator2::process_with_gate );
		}
		else
		{
			bool prevTrigger = trigger_state;
			bool prevGate = gate_state;

			gate_state = ( pinGate > 0.f );
			trigger_state = ( pinTrigger > 0.f );

			// Gate must be high, and either gate or trigger transition high.
			if( gate_state && ( !prevGate || (trigger_state && !prevTrigger)) )
			{
				NoteOn( -1 );
			}
		}
	}

	if( pinPitch.isUpdated() )
	{
		ChooseSubProcess(-1);
	}
}

void SoundfontOscillator2::sub_process_silence(int bufferOffset, int sampleframes)
{
	float* output_l  = bufferOffset + pinLeft.getBuffer();	
	float* output_r  = bufferOffset + pinRight.getBuffer();	

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*output_l++ = *output_r++ = 0.f;
	}
}

void SoundfontOscillator2::process_with_gate(int bufferOffset, int sampleframes)
{
	float* gate  = bufferOffset + pinGate.getBuffer();	
	float* trigger  = bufferOffset + pinTrigger.getBuffer();	

	int last = bufferOffset + sampleframes;
	int to_pos = bufferOffset;
	int cur_pos = bufferOffset;

	while( cur_pos < last )
	{
		// how long till next gate/trigger change?
		while( (*gate > 0.f ) == gate_state && (*trigger > 0.f ) == trigger_state && to_pos < last )
		{
			to_pos++;
			gate++;
			trigger++;
		}

		//		if( to_pos > cur_pos ) // no harm (sub_process mono seems to handle sampleframes = 0 ok)
		(this->*(current_osc_func))( cur_pos, to_pos - cur_pos );

		if( to_pos == last )
		{
			return;
		}

		cur_pos = to_pos;

		bool prevTrigger = trigger_state;
		bool prevGate = gate_state;

		gate_state = ( *gate > 0.f );
		trigger_state = ( *trigger > 0.f );

		// Gate must be high, and either gate or trigger transition high.
		if( gate_state && ( !prevGate || (trigger_state && !prevTrigger)) )
		{
			NoteOn( cur_pos );
		}
	}
}


void SoundfontOscillator2::NoteOn( int blockPosistion )
{
	sample_playing = false;

	if( sampleHandle != pinSampleId )
	{
		partials.clear();

		SampleManager::Instance()->Release( sampleHandle );
		sampleHandle = pinSampleId;

		if( sampleHandle != -1 ) // none loaded
		{
			SampleManager::Instance()->AddRef( sampleHandle );
		}
	}

	if( sampleHandle != -1 ) // loaded OK.
	{
		float p_pitch = pinPitch.getValue( blockPosistion );
		float p_velocity = pinVelocity.getValue( blockPosistion );

		const int MIDDLE_A = 69;
		WORD bank = 0;
		int chan = 0;
		float NoteNum = floorf(0.5f + MIDDLE_A + (p_pitch - 0.5f) * 120.f);
		int NoteVel = (int) (p_velocity * 127.f);

		if( NoteNum < 0.f )
			 NoteNum = 0.f;

		if( NoteNum > 127.f )
			 NoteNum = 127.f;

		if( NoteVel < 0 )
			 NoteVel = 0;

		if( NoteVel > 127 )
			 NoteVel = 127;

		// scan zone list
	//	_RPT3(_CRT_WARN, "Note On Note %d, Vel %d, chan %d\n",(int)NoteNum,(int)NoteVel,(int)0 );

		GetZone(chan, (int)NoteNum, NoteVel);

		for( auto& partial : partials )
		{
			Jzone &z = *partial.zone;

			assert(0 == (partial.cur_sample_r->sfSampleType & 0x8000)); // no ROM samples allowed.

/*todo			if( ((SoundfontOscillator2*) CloneOf())->m_status != 0 )
			{
				((SoundfontOscillator2*) CloneOf())->m_status = 0; // Good Sample
				Refresh_UI();
			}
*/
			sample_playing = true;

			float root_key = partial.cur_sample_r->byOriginalKey;
			float pitch_correction = partial.cur_sample_r->chCorrection;

			float overiding_root_key = z.Get(58).shAmount;
			if( overiding_root_key >= 0.0 )
				root_key = overiding_root_key;

			float course_tune = z.Get(51).shAmount;
			float fine_tune = z.Get(52).shAmount;
			partial.scale_tune = z.Get(56).shAmount * 0.1f; // degree to which MIDI key number influences pitch. zero -> no effect on pitch. 100 - normal semitone scale
			float tune = course_tune + ( fine_tune + pitch_correction ) / 100.f;
			/*
				_RPT3(_CRT_WARN, "root_key %f, course_tune %f fine_tune %f\n", root_key, course_tune,fine_tune, );
				_RPT1(_CRT_WARN, "sample rate %d\n", cur_sample_r->dwSampleRate );
				_RPT1(_CRT_WARN, "bend %f\n", bend );
			*/
			float transposition = ( (float) NoteNum - root_key + tune ) / 120.f ;
			
			partial.root_pitch = 0.5f + (NoteNum - MIDDLE_A) / 120.f;
			partial.root_pitch -= transposition;

			partial.relative_sample_rate = (float) partial.cur_sample_r->dwSampleRate / getSampleRate();
			partial.CalculateIncrement( p_pitch );
			partial.s_ptr_fine = -partial.s_increment;
		}
	}

	/*
	if( zone_list->GetUpperBound() == -1 )
	{
		_RPT0(_CRT_WARN, "Sample Player, EMPTY PATCH\n" );
	}
	else
	{
		_RPT2(_CRT_WARN,"Sample Play:no suitable sample (Note %d, Vel %d)\n",  NoteNum,  NoteVel);
	}
*/

	pinLeft.setStreaming( sample_playing, blockPosistion );
	pinRight.setStreaming( sample_playing, blockPosistion );

	ChooseSubProcess( blockPosistion );
}

void SoundfontOscillator2::NoteDone( int blockPosistion )
{
	pinRight.setStreaming( false, blockPosistion );
	pinLeft.setStreaming( false, blockPosistion );

	sample_playing = false;

	SampleManager::Instance()->Release( sampleHandle );
	sampleHandle = -1;

	ChooseSubProcess( blockPosistion );

//TODO	cur_zone = 0; // prevent crash trying to access it
//	_RPT0(_CRT_WARN, "\nNote Done\n" );
}

int32_t SoundfontOscillator2::open()
{
	interpolation_table2 = GetInterpolationtable();
	SincInterpolator::interpolation_table2 = GetInterpolationtable();

	return MpBase::open();
}

SoundfontOscillator2::~SoundfontOscillator2()
{
	SampleManager::Instance()->Release( sampleHandle );
}

void SoundfontOscillator2::ChooseSubProcess( int blockPosistion )
{
	if( sample_playing )
	{
		if( pinPitch.isStreaming() )
		{
			bool usesPanning = false;
			for( activePartialListType::iterator it = partials.begin() ; it != partials.end() ; ++it )
			{
				partial &partial = *it;
                usesPanning = (std::max)( usesPanning, partial.UsesPanning() );
			}
			// if( cur_sample_l ) // no mono-optimised loop yet.
			{
				switch( pinQuality )
				{
					case 8:
						if( usesPanning )
						{
							// current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_pitch_stereo_panned_hi_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< SincInterpolator, PitchChanging, Panning >;
						}
						else
						{
							// current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_pitch_stereo_hi_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< SincInterpolator, PitchChanging, noPanning >;
						}
						break;

					case 4:
						if( usesPanning )
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_pitch_stereo_panned_mid_quality;
							//CodeGenerator<CubicInterpolator,PitchChanging, Panning>::sub_process( *this, bufferOffset, sampleframes );
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< CubicInterpolator, PitchChanging, Panning >;
						}
						else
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_pitch_stereo_mid_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< CubicInterpolator, PitchChanging, noPanning >;
						}
						break;

					case 0:
						if( usesPanning )
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_pitch_stereo_panned_no_quality;
							//CodeGenerator<NoInterpolator,PitchChanging, Panning>::sub_process( *this, bufferOffset, sampleframes );
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< NoInterpolator, PitchChanging, Panning >;
						}
						else
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_pitch_stereo_no_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< NoInterpolator, PitchChanging, noPanning >;
						}
						break;

					default: // 2
						if( usesPanning )
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_pitch_stereo_panned_low_quality;
							//CodeGenerator<LinearInterpolator,PitchChanging, Panning>::sub_process( *this, bufferOffset, sampleframes );
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< LinearInterpolator, PitchChanging, Panning >;
						}
						else
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_pitch_stereo_low_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< LinearInterpolator, PitchChanging, noPanning >;
						}
						break;
				}
			}
			/*
			else
			{
				switch( pinQuality )
				{
					case 8:
						current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_pitch_mono_hi_quality;
						break;

					case 4:
						current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_pitch_mono_mid_quality;
						break;

					default:
						current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_pitch_mono_low_quality;
						break;
				}
			}
			*/
		}
		else
		{
			bool usesPanning = false;

			for( activePartialListType::iterator it = partials.begin() ; it != partials.end() ; ++it )
			{
				partial &partial = *it;
				partial.CalculateIncrement( pinPitch.getValue( blockPosistion ) );

                usesPanning = (std::max)( usesPanning, partial.UsesPanning() );
			}

			// if( cur_sample_l ) // no mono-optimised loop yet.
			{
				int test = pinQuality;
				switch( pinQuality )
				{
					case 8:
						if( usesPanning )
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_stereo_panned_hi_quality;
							// CodeGenerator<SincInterpolator, PitchFixed, Panning>::sub_process( *this, bufferOffset, sampleframes );
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< SincInterpolator, PitchFixed, Panning >;
						}
						else
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_stereo_hi_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< SincInterpolator, PitchFixed, noPanning >;
						}
						break;

					case 4:
						if( usesPanning )
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_stereo_panned_mid_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< CubicInterpolator, PitchFixed, Panning >;
						}
						else
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_stereo_mid_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< CubicInterpolator, PitchFixed, noPanning >;
						}
						break;

					case 0:
						if( usesPanning )
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_stereo_panned_no_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< NoInterpolator, PitchFixed, Panning >;
						}
						else
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_stereo_no_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< NoInterpolator, PitchFixed, noPanning >;
						}
						break;

					default: // 2
						if( usesPanning )
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_stereo_panned_low_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< LinearInterpolator, PitchFixed, Panning >;
						}
						else
						{
							//current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_stereo_low_quality;
							current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_template< LinearInterpolator, PitchFixed, noPanning >;
						}
						break;
				}
			}
			/*
			else
			{
				current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_mono;
			}
			*/
		}
	}
	else
	{
		current_osc_func = (SubProcess_ptr) &SoundfontOscillator2::sub_process_silence;
	}

	if( pinGate.isStreaming() )
	{
		SET_PROCESS( &SoundfontOscillator2::process_with_gate );
	}
	else
	{
		SET_PROCESS( current_osc_func );
	}
}

void SoundfontOscillator2::ResetWave()
{
	if( sample_playing )
	{
		NoteDone( -1 );
		NoteOn( -1 );
	}
}

inline bool is_denormal( float f )
{
	uint32_t l = *((uint32_t*)&f);

	return( f != 0.f && (l & 0x7FF00000) == 0 && (l & 0x000FFFFF) != 0 ); // anything less than approx 1E-38 excluding +ve and -ve zero (two distinct values)
}

// *** shared Interpolation filter setup ***
float* SoundfontOscillator2::GetInterpolationtable()
{
	if( interpolationTableInitialized )
		return interpolationTable;

	{
		// create a pointer to array with 'negative' indices
		interpolation_table2 = interpolationTable;
		// initialise interpolation table
		// as per page 523 MAMP
		// create vitual array with -ve indices

		// check not initialised already, entry 11 should be 0.801....
		assert( fabs(interpolation_table2[11] - .801) > .1 );

		int i;
		const int table_width = INTERPOLATION_POINTS / 2;
		const int table_entries = ( table_width + 1 ) * INTERPOLATION_DIV;
		const float PI = 3.14159265358979323846f;

		for( int sub_table = 0 ; sub_table < INTERPOLATION_DIV ; sub_table++ )
		{
			int table_index = sub_table * INTERPOLATION_POINTS + INTERPOLATION_POINTS/2 - 1;
			for( int x = -table_width ; x < table_width; x++ )
			{
				i = sub_table + x * INTERPOLATION_DIV;
				// position on x axis
				double o = (double) i / INTERPOLATION_DIV;
				// filter impulse response
				double sinc = sin( PI * o ) / ( PI * o );
				
				// apply tailing function
				double hanning = cos( 0.5 * PI * i / ( INTERPOLATION_DIV * table_width ) );
				float windowed_sinc = (float)(sinc * hanning * hanning);

				assert( (table_index-x) >= 0 && (table_index-x) < tableSize_ );
				interpolation_table2[table_index-x] = windowed_sinc;
			}
		}
		assert( (table_width-1) >= 0 && (table_width-1) < tableSize_ );
		interpolation_table2[table_width-1] = 1.f; // fix div by 0 bug

		// first table copied to last, shifted 1 place
		int idx = INTERPOLATION_DIV * INTERPOLATION_POINTS;
		for( int table_entry = 1 ; table_entry <= INTERPOLATION_POINTS ; table_entry++ )
		{
			assert( (idx + table_entry) >= 0 && (idx + table_entry) < tableSize_ );
			interpolation_table2[idx + table_entry] = interpolation_table2[table_entry-1];
		}
//		interpolation_table2[idx+INTERPOLATION_POINTS-1] = 0.f;
		assert( (idx) >= 0 && (idx) < tableSize_ );
		interpolation_table2[idx] = 0.f;

/* print out interpolation table
		for( int sub_table = 0 ; sub_table <= INTERPOLATION_DIV ; sub_table++ )
		{
			for( int x = 0 ; x < INTERPOLATION_POINTS; x++ )
			{
				int table_index = sub_table * INTERPOLATION_POINTS + x;
				int linear_index = sub_table + INTERPOLATION_DIV * x;
				_RPT2(_CRT_WARN,"%d %+.5f\n",linear_index, interpolation_table2[table_index] );
			}
		}
*/
		// additional fine tuning.  Normalise all 32 posible filters so total gain is always 1.0
		// This fixes 'overtones' problems, due to different sub-filters having slightly different overall gains
		for( int sub_table = 0 ; sub_table <= INTERPOLATION_DIV ; sub_table++ )
		{
			int table_index = sub_table * INTERPOLATION_POINTS;
			assert( table_index >= 0 && table_index < table_entries * 2);
			// calc sub table sum
			double fir_sum = 0.f;
			for( int table_entry = 0 ; table_entry < INTERPOLATION_POINTS ; table_entry++ )
			{
				fir_sum += interpolation_table2[table_index+table_entry];
			}

			// use it to normalise sub table
			for( int table_entry = 0 ; table_entry < INTERPOLATION_POINTS ; table_entry++ )
			{
				float adjusted = interpolation_table2[table_index+table_entry] / (float)fir_sum;
				if( is_denormal(adjusted))
					adjusted = 0.f;
				assert( (table_index+table_entry) >= 0 && (table_index+table_entry) < tableSize_ );
				interpolation_table2[table_index+table_entry] = adjusted;
			}
		}
	}

	interpolationTableInitialized = true;
	return interpolationTable;
}

