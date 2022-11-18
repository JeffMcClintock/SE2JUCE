#include "./SvFilter2.h"
#include "../shared/xplatform.h"

REGISTER_PLUGIN ( SvFilter2, L"SE SV Filter2" );

#define MAX_VOLTS ( 10.f )
#define FSampleToVoltage(s) ( (float) (s) * (float) MAX_VOLTS)
#define FSampleToFrequency(volts) ( 440.f * powf(2.f, FSampleToVoltage(volts) - (float) MAX_VOLTS / 2.f ) )
inline static unsigned int FrequencyToIntIncrement( float sampleRate, double freq_hz )
{
	double temp_float = (/*UINT_MAX*/ 0xffffffff + 1.f) * freq_hz / sampleRate + 0.5f;
	return (unsigned int) temp_float;
}

inline float ComputeIncrement( float SampleRate, float pitch )
{
	return FSampleToFrequency( pitch ) / SampleRate;
}


SvFilter2::SvFilter2( IMpUnknown* host ) : MpBase( host )
	,low_pass1(0.0f)
	,band_pass1(0.0f)
	,low_pass2(0.0f)
	,band_pass2(0.0f)
	,stabilityCheckCounter(0)
	,outputQuiet(false)
{
	// Register pins.
	initializePin( 0, pinSignal );
	initializePin( 1, pinPitch );
	initializePin( 2, pinResonance );
	initializePin( 3, pinStrength );
	initializePin( 4, pinMode );
	initializePin( 5, pinOutput );
}

int32_t SvFilter2::open()
{
	// 20kHz is about 10.5 Volts. 1Hz is about -3.7 volts. 0.01Hz = -10V
	// -4 -> 11 Volts should cover most posibilities. 15V Range. 12 entries per volt = 180 entries.
	const int entriesPerOctave = 12;
	const int extraEntriesAtStart = 1; // for interpolator.
	const int extraEntriesAtEnd = 3; // for interpolator.
	const int pitchTableSize = extraEntriesAtStart + extraEntriesAtEnd + (pitchTableHiVolts - pitchTableLowVolts) * entriesPerOctave;
//	const float oneSemitone = 1.0f/12.0f;
	int size = (pitchTableSize) * sizeof(float);
	int32_t needInitialize;
	getHost()->allocateSharedMemory( L"SvFilter2:Pitch", (void**) &pitchTable, getSampleRate(), size, needInitialize );

	if( needInitialize )
	{
		for( int i = 0 ; i < pitchTableSize ; ++i )
		{
			float pitch = (pitchTableLowVolts + (i - extraEntriesAtStart) / (float) entriesPerOctave);// * 0.1f;
			float hz = 440.f * powf(2.f, (pitch) - 5.f );

			pitchTable[i] = 2.0f * (float)M_PI * hz / getSampleRate();
			// _RPT3(_CRT_WARN, "%3d %f %f\n", i, pitch, pitchTable[i] );
		}
	}

	pitchTable += extraEntriesAtStart; // Shift apparent start of table to entry #1, so we can access table[-1] without hassle.
	pitchTable -= pitchTableLowVolts * entriesPerOctave; // Shift apparent start of table Volts = 0.0.

	// init max freq lookup table (given reson, lookup max freq filter can handle)
	size = 514;
	getHost()->allocateSharedMemory( L"SvFilter2:StabilityLimit", (void**) &stabilityTable, getSampleRate(), size * sizeof(float), needInitialize );
	if( needInitialize )
	{
		for( int j = 0 ; j < size ; j++ )
		{
			// orig, too harsh			max_freq_lookup->SetValue( j,VoltageToSample( 0.000025 * j*j + 0.009 * j + 8.2378) );
			float temp = (0.000025f * j*j + 0.009f * j + 7.8f);
			stabilityTable[j] = temp * 0.1f;
		}
	}

	return MpBase::open();
}

void SvFilter2::onSetPins(void)
{
	// Check which pins are updated.
	if( pinResonance.isUpdated() && !pinResonance.isStreaming() )
	{
		ResonanceFixed::CalcInitial( stabilityTable, pinResonance, maxStableF1, quality );
	}

	if( pinPitch.isUpdated() && !pinPitch.isStreaming() )
	{
		PitchFixed::CalcInitial( pitchTable, pinPitch, maxStableF1, f1 );
	}

	if( pinMode.isUpdated() )
	{
	}

	// Set state of output audio pins.
	pinOutput.setStreaming(true);
	outputQuiet = false;

	// Set processing method.

	typedef void (SvFilter2::* MyProcess_ptr)(int bufferOffset, int sampleFrames);
	#define PROCESS_PTR( pitch, resonance, mode, stages ) &SvFilter2::subProcess< pitch, resonance, mode, stages >
	const static MyProcess_ptr ProcessSelection[2][2][4][2] =
	{
		PROCESS_PTR( PitchFixed,    ResonanceFixed,    FilterModeLowPass    , 1),
		PROCESS_PTR( PitchFixed,    ResonanceFixed,    FilterModeLowPass    , 2),
		PROCESS_PTR( PitchFixed,    ResonanceFixed,    FilterModeHighPass   , 1),
		PROCESS_PTR( PitchFixed,    ResonanceFixed,    FilterModeHighPass   , 2),
		PROCESS_PTR( PitchFixed,    ResonanceFixed,    FilterModeBandPass   , 1),
		PROCESS_PTR( PitchFixed,    ResonanceFixed,    FilterModeBandPass   , 2),
		PROCESS_PTR( PitchFixed,    ResonanceFixed,    FilterModeBandReject , 1),
		PROCESS_PTR( PitchFixed,    ResonanceFixed,    FilterModeBandReject , 2),
		0, // PROCESS_PTR( PitchFixed,    ResonanceChanging, FilterModeLowPass    , 1),
		0, // PROCESS_PTR( PitchFixed,    ResonanceChanging, FilterModeLowPass    , 2),
		0, // PROCESS_PTR( PitchFixed,    ResonanceChanging, FilterModeHighPass   , 1),
		0, // PROCESS_PTR( PitchFixed,    ResonanceChanging, FilterModeHighPass   , 2),
		0, // PROCESS_PTR( PitchFixed,    ResonanceChanging, FilterModeBandPass   , 1),
		0, // PROCESS_PTR( PitchFixed,    ResonanceChanging, FilterModeBandPass   , 2),
		0, // PROCESS_PTR( PitchFixed,    ResonanceChanging, FilterModeBandReject , 1),
		0, // PROCESS_PTR( PitchFixed,    ResonanceChanging, FilterModeBandReject , 2),
		PROCESS_PTR( PitchChanging, ResonanceFixed,    FilterModeLowPass    , 1),
		PROCESS_PTR( PitchChanging, ResonanceFixed,    FilterModeLowPass    , 2),
		PROCESS_PTR( PitchChanging, ResonanceFixed,    FilterModeHighPass   , 1),
		PROCESS_PTR( PitchChanging, ResonanceFixed,    FilterModeHighPass   , 2),
		PROCESS_PTR( PitchChanging, ResonanceFixed,    FilterModeBandPass   , 1),
		PROCESS_PTR( PitchChanging, ResonanceFixed,    FilterModeBandPass   , 2),
		PROCESS_PTR( PitchChanging, ResonanceFixed,    FilterModeBandReject , 1),
		PROCESS_PTR( PitchChanging, ResonanceFixed,    FilterModeBandReject , 2),
		PROCESS_PTR( PitchChanging, ResonanceChanging, FilterModeLowPass    , 1),
		PROCESS_PTR( PitchChanging, ResonanceChanging, FilterModeLowPass    , 2),
		PROCESS_PTR( PitchChanging, ResonanceChanging, FilterModeHighPass   , 1),
		PROCESS_PTR( PitchChanging, ResonanceChanging, FilterModeHighPass   , 2),
		PROCESS_PTR( PitchChanging, ResonanceChanging, FilterModeBandPass   , 1),
		PROCESS_PTR( PitchChanging, ResonanceChanging, FilterModeBandPass   , 2),
		PROCESS_PTR( PitchChanging, ResonanceChanging, FilterModeBandReject , 1),
		PROCESS_PTR( PitchChanging, ResonanceChanging, FilterModeBandReject , 2),
	};

	// If reson is moduled, have to treat freq as if it's modulated (as maximum stabel freq is therefore modulated).
	setSubProcess(static_cast <SubProcess_ptr> ( ProcessSelection[ pinPitch.isStreaming() || pinResonance.isStreaming() ][ pinResonance.isStreaming() ][pinMode][pinStrength] ));

#if 0
	// Print out the table.
		for( int a = 0 ; a < 2 ; ++a )
		{
			for( int b = 0 ; b < 2 ; ++b )
			{
				for( int c = 0 ; c < 4 ; ++c )
				{
					for( int d = 0 ; d < 2 ; ++d )
					{
//						for( int e = 0 ; e < 2 ; ++e )
						{
//							for( int f = 0 ; f < 2 ; ++f )
							{
								_RPT0(_CRT_WARN, "PROCESS_PTR( ");

								if( a == 0 )
								{
									_RPT0(_CRT_WARN, "PitchFixed,    ");
								}
								else
								{
									_RPT0(_CRT_WARN, "PitchChanging, ");
								}

								if( b == 0 )
								{
									_RPT0(_CRT_WARN, "ResonanceFixed,    ");
								}
								else
								{
									_RPT0(_CRT_WARN, "ResonanceChanging, ");
								}

								switch( c )
								{
								case 0:
									_RPT0(_CRT_WARN, "FilterModeLowPass    ");
									break;
								case 1:
									_RPT0(_CRT_WARN, "FilterModeHighPass   ");
									break;
								case 2:
									_RPT0(_CRT_WARN, "FilterModeBandPass   ");
									break;
								case 3:
									_RPT0(_CRT_WARN, "FilterModeBandReject ");
									break;
								}

								if( d == 0 ) // filter stages: 1 or 2.
								{
									_RPT0(_CRT_WARN, ", 1");
								}
								else
								{
									_RPT0(_CRT_WARN, ", 2");
								}
									
								_RPT0(_CRT_WARN, "),\n");
							}
						}
					}
				}
			}
		}
#endif

	// If no input signal, check more often for filter to settle.
	if( !pinSignal.isStreaming() && !pinPitch.isStreaming() && !pinResonance.isStreaming() )
	{
		stabilityCheckCounter = std::max( stabilityCheckCounter, 2 );
	}
}

void SvFilter2::StabilityCheck( int blockPosition )
{
	int blockSize = getBlockSize();
	int i = blockPosition;
	int iPrev = (i - 1 + blockSize) % blockSize;
	float currentOutput = pinOutput.getBuffer()[i];
	float previousOutput = pinOutput.getBuffer()[iPrev];

	// If no input signal, check more often for filter to settle.
	if( !pinSignal.isStreaming() && !pinPitch.isStreaming() && !pinResonance.isStreaming() )
	{
		stabilityCheckCounter = std::max( stabilityCheckCounter, 1 );
	}
	else
	{
		stabilityCheckCounter = 50;
	}

	//	_RPT1(_CRT_WARN, "ug_filter_sv::OverflowCheck() %d\n", SampleClock() );
	// periodic check/correct for numeric overflow
	if( ! isfinite( low_pass1 ) ) // overload?
	{
		low_pass1 = 0.f;
	}

	if( ! isfinite( band_pass1 ) ) // overload?
	{
		band_pass1 = 0.f;
	}

	// check for output quiet (means we can sleep)
	if( outputQuiet == false )
	{
		if( !pinSignal.isStreaming() ) // input stopped
		{
			// rough estimate of energy in filter. Difficult to use as non-zero fixed inputs produce signifigant 'enery' due to numeric errors
			float energy = fabsf(low_pass1 - currentOutput) + fabsf(band_pass1);

			const float INSIGNIFICANT_SAMPLE = 0.000001f;
			if( currentOutput == previousOutput || energy < INSIGNIFICANT_SAMPLE )
			{
				low_pass1 = currentOutput;
				band_pass1 = 0.f;
				outputQuiet = true;

				SET_PROCESS( &SvFilter2::subProcessStatic );
				pinOutput.setStreaming(false, blockPosition );
			}
		}
	}
}