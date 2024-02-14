#include "./SvFilter2.h"

REGISTER_PLUGIN2( SvFilter2, L"SE SV Filter2" );

SvFilter2::SvFilter2() :
	low_pass1(0.0f)
	,band_pass1(0.0f)
	,low_pass2(0.0f)
	,band_pass2(0.0f)
{
}

void SvFilter2::initializePins()
{
	// Register pins.
	initializePin(pinSignal);
	initializePin(pinPitch);
	initializePin(pinResonance);
	initializePin(pinStrength);
	initializePin(pinMode);
	initializePin(pinOutput);
}

int32_t SvFilter2::open()
{
	initializePins();

	// 20kHz is about 10.5 Volts. 1Hz is about -3.7 volts. 0.01Hz = -10V
	// -4 -> 11 Volts should cover most posibilities. 15V Range. 12 entries per volt = 180 entries.
	const int entriesPerOctave = 12;
	const int extraEntriesAtStart = 1; // for interpolator.
	const int extraEntriesAtEnd = 3; // for interpolator.
	const int pitchTableSize = extraEntriesAtStart + extraEntriesAtEnd + (pitchTableHiVolts - pitchTableLowVolts) * entriesPerOctave;

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

	return FilterBase::open();
}

void SvFilter2::onSetPins()
{
	// Check which pins are updated.
	if (pinResonance.isUpdated() && !pinResonance.isStreaming())
	{
		ResonanceFixed::CalcInitial(stabilityTable, pinResonance, maxStableF1, quality);
		f1 = (std::min)(f1, maxStableF1);
	}

	if (pinPitch.isUpdated() && !pinPitch.isStreaming())
	{
		PitchFixed::CalcInitial(pitchTable, pinPitch, maxStableF1, f1);
	}

	ChoseProcessMethod();

	initSettling();
}

void SvFilter2::ChoseProcessMethod()
{
	// Set state of output audio pins.
	pinOutput.setStreaming(true);

	// Set processing method.

	typedef void (SvFilter2::* MyProcess_ptr)(int sampleFrames);
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
	setSubProcess(ProcessSelection[ pinPitch.isStreaming() || pinResonance.isStreaming() ][ pinResonance.isStreaming() ][pinMode][pinStrength]);

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
}

// periodic check/correct for numeric overflow
void SvFilter2::StabilityCheck()
{
	for (auto* state : { &low_pass1, &band_pass1, &low_pass2, &band_pass2 })
	{
		if (isnan(*state) || !isfinite(*state)) // overload?
	{
			*state = 0.f;
		}
	}
}