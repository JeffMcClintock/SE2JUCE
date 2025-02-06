#include <float.h>
#include <climits>
#include <mutex>
#include "./SvFilter.h"

REGISTER_PLUGIN2(StateVariableFilter3, L"SE SV Filter4");
REGISTER_PLUGIN2(StateVariableFilter3B, L"SE SV Filter4B");
SE_DECLARE_INIT_STATIC_FILE(SVFilter4);
SE_DECLARE_INIT_STATIC_FILE(SVFilter4B);

StateVariableFilter3B::StateVariableFilter3B()
{
	initializePin(pinSignal);
	initializePin(pinPitch);
	initializePin(pinResonance);
	initializePin(pinOutput);
	initializePin(pinHiPass);
	initializePin(pinBandPass);
	initializePin(pinBandReject);
}

void StateVariableFilter3B::ChoseProcessMethod()
{
	pinOutput.setStreaming(true);
	pinHiPass.setStreaming(true);
	pinBandPass.setStreaming(true);
	pinBandReject.setStreaming(true);

	// Set processing method.
	typedef void ( StateVariableFilter3B::* MyProcess_ptrB )( int sampleFrames );
	#define PROCESS_PTRB( pitch, resonance ) &StateVariableFilter3B::subProcessZdf< pitch, resonance >

	// NOTE: Resonance limits depend on pitch, so when pitch modulated, need to calc resonance too.
	const static MyProcess_ptrB ProcessSelection[2][2] =
	{
		PROCESS_PTRB(FilterPitchFixed, ResonanceFixed),
		PROCESS_PTRB(FilterPitchFixed, ResonanceChanging),
		nullptr,
		PROCESS_PTRB(FilterPitchChanging, ResonanceChanging),
	};

	// If reson is moduled, have to treat freq as if it's modulated (as maximum stabel freq is therefore modulated).
	setSubProcess(ProcessSelection[pinPitch.isStreaming()][pinPitch.isStreaming() || pinResonance.isStreaming()] );
}

void StateVariableFilter3B::OnFilterSettled()
{
	SET_PROCESS2(&StateVariableFilter3B::subProcessStatic);

	const int blockPosition = getBlockPosition(); // assuming StabilityCheck() always called at start of block.
	pinOutput.setStreaming(false, blockPosition);
	pinHiPass.setStreaming(false, blockPosition);
	pinBandPass.setStreaming(false, blockPosition);
	pinBandReject.setStreaming(false, blockPosition);
}

StateVariableFilter3::StateVariableFilter3()
{
	// Register pins.
	initializePin( pinSignal );
	initializePin( pinPitch );
	initializePin( pinResonance );
	initializePin( pinStrength );
	initializePin( pinMode );
	initializePin( pinOutput );
}

void StateVariableFilter3::ChoseProcessMethod()
{
	pinOutput.setStreaming(true);

	typedef void ( StateVariableFilter3::* MyProcess_ptr )( int sampleFrames );
#define PROCESS_PTR( pitch, resonance, mode, stages ) &StateVariableFilter3::subProcessZdf< pitch, resonance, mode, stages >

	// NOTE: Resonance limits depend on pitch, so when pitch modulated, need to calc resonance too.
	const static MyProcess_ptr ProcessSelection[2][2][4][2] =
	{
		PROCESS_PTR(FilterPitchFixed, ResonanceFixed, FilterModeLowPass, 1),
		PROCESS_PTR(FilterPitchFixed, ResonanceFixed, FilterModeLowPass, 2),
		PROCESS_PTR(FilterPitchFixed, ResonanceFixed, FilterModeHighPass, 1),
		PROCESS_PTR(FilterPitchFixed, ResonanceFixed, FilterModeHighPass, 2),
		PROCESS_PTR(FilterPitchFixed, ResonanceFixed, FilterModeBandPass, 1),
		PROCESS_PTR(FilterPitchFixed, ResonanceFixed, FilterModeBandPass, 2),
		PROCESS_PTR(FilterPitchFixed, ResonanceFixed, FilterModeBandReject, 1),
		PROCESS_PTR(FilterPitchFixed, ResonanceFixed, FilterModeBandReject, 2),
		PROCESS_PTR(FilterPitchFixed, ResonanceChanging, FilterModeLowPass, 1),
		PROCESS_PTR(FilterPitchFixed, ResonanceChanging, FilterModeLowPass, 2),
		PROCESS_PTR(FilterPitchFixed, ResonanceChanging, FilterModeHighPass, 1),
		PROCESS_PTR(FilterPitchFixed, ResonanceChanging, FilterModeHighPass, 2),
		PROCESS_PTR(FilterPitchFixed, ResonanceChanging, FilterModeBandPass, 1),
		PROCESS_PTR(FilterPitchFixed, ResonanceChanging, FilterModeBandPass, 2),
		PROCESS_PTR(FilterPitchFixed, ResonanceChanging, FilterModeBandReject, 1),
		PROCESS_PTR(FilterPitchFixed, ResonanceChanging, FilterModeBandReject, 2),
		0, // PROCESS_PTR( FilterPitchChanging, ResonanceFixed,    FilterModeLowPass    , 1), // Not used.
		0, // PROCESS_PTR( FilterPitchChanging, ResonanceFixed,    FilterModeLowPass    , 2),
		0, // PROCESS_PTR( FilterPitchChanging, ResonanceFixed,    FilterModeHighPass   , 1),
		0, // PROCESS_PTR( FilterPitchChanging, ResonanceFixed,    FilterModeHighPass   , 2),
		0, // PROCESS_PTR( FilterPitchChanging, ResonanceFixed,    FilterModeBandPass   , 1),
		0, // PROCESS_PTR( FilterPitchChanging, ResonanceFixed,    FilterModeBandPass   , 2),
		0, // PROCESS_PTR( FilterPitchChanging, ResonanceFixed,    FilterModeBandReject , 1),
		0, // PROCESS_PTR( FilterPitchChanging, ResonanceFixed,    FilterModeBandReject , 2),
		PROCESS_PTR(FilterPitchChanging, ResonanceChanging, FilterModeLowPass, 1),
		PROCESS_PTR(FilterPitchChanging, ResonanceChanging, FilterModeLowPass, 2),
		PROCESS_PTR(FilterPitchChanging, ResonanceChanging, FilterModeHighPass, 1),
		PROCESS_PTR(FilterPitchChanging, ResonanceChanging, FilterModeHighPass, 2),
		PROCESS_PTR(FilterPitchChanging, ResonanceChanging, FilterModeBandPass, 1),
		PROCESS_PTR(FilterPitchChanging, ResonanceChanging, FilterModeBandPass, 2),
		PROCESS_PTR(FilterPitchChanging, ResonanceChanging, FilterModeBandReject, 1),
		PROCESS_PTR(FilterPitchChanging, ResonanceChanging, FilterModeBandReject, 2),
	};

	// If resonance is modulated, have to treat freq as if it's modulated too (because maximum stable freq is therefore modulated).
	setSubProcess(static_cast <SubProcess_ptr2> ( ProcessSelection[pinPitch.isStreaming()][pinPitch.isStreaming() || pinResonance.isStreaming()][pinMode][pinStrength] ));
}

void StateVariableFilter3::OnFilterSettled()
{
	if (pinMode != 0 && pinMode != 3) // LP, BR
	{
		static_output = 0.0f;
	}

	FilterBase::OnFilterSettled();
}

int32_t StateVariableFilterBase::open()
{
	// fix for race conditions.
	static std::mutex safeInit;
	std::lock_guard<std::mutex> lock(safeInit);

	// 20kHz is about 10.5 Volts. 1Hz is about -3.7 volts. 0.01Hz = -10V
	// -4 -> 11 Volts should cover most posibilities. 15V Range. 12 entries per volt = 180 entries.
	const int entriesPerOctave = 12;
	const int extraEntriesAtStart = 1; // for interpolator.
	const int extraEntriesAtEnd = 3; // for interpolator.
	const int pitchTableSize = extraEntriesAtStart + extraEntriesAtEnd + ( FilterPitchChanging::pitchTableHiVolts - FilterPitchChanging::pitchTableLowVolts ) * entriesPerOctave;
	int size = (pitchTableSize)* sizeof(float);
	int32_t needInitialize;
	getHost()->allocateSharedMemory(L"StateVariableFilter3:Pitch", (void**)&pitchTable, getSampleRate(), size, needInitialize);

	if( needInitialize )
	{
		float T = 1.0f / getSampleRate();
		for( int i = 0; i < pitchTableSize; ++i )
		{
			float pitch = FilterPitchChanging::pitchTableLowVolts + ( i - extraEntriesAtStart ) / (float)entriesPerOctave;
			float hz = 440.f * powf(2.f, pitch - 5.f);
			hz = ( std::min )( hz, getSampleRate() * 0.49f ); // stability limit.

			// prewarp the cutoff- these are bilinear-transform filters
			float wd = 2.0f * (float)M_PI * hz;
			float wa1 = ( 2.0f / T )*tan(wd * T / 2.0f);
			float g = wa1 * T / 2.0f;

			pitchTable[i] = g;
			// _RPT3(_CRT_WARN, "%3d %f %f\n", i, pitch, pitchTable[i] );
		}
	}

	pitchTable += extraEntriesAtStart; // Shift apparent start of table to entry #1, so we can access table[-1] without hassle.
	pitchTable -= FilterPitchChanging::pitchTableLowVolts * entriesPerOctave; // Shift apparent start of table Volts = 0.0.

	return FilterBase::open();
}

void StateVariableFilterBase::onSetPins()
{
	// Check which pins are updated.
	if( pinPitch.isUpdated() && !pinPitch.isStreaming() )
	{
		FilterPitchFixed::CalcInitial(pitchTable, pinPitch, coef_g);
		ResonanceFixed::CalcInitial(pinResonance, pinPitch, coef_R); // Resonance stability depends on pitch.
	}
	else
	{
		if( pinResonance.isUpdated() && !pinResonance.isStreaming() )
		{
			ResonanceFixed::CalcInitial(pinResonance, pinPitch, coef_R);
		}
	}

	ChoseProcessMethod();
	initSettling();
}

void StateVariableFilterBase::StabilityCheck()
{
	// periodic check/correct for numeric overflow.
	if (!isfinite(m_fZA1) || !isfinite(m_fZB1) || !isfinite(m_fZA2) || !isfinite(m_fZB2))
	{
		m_fZA1 = m_fZB1 = m_fZA2 = m_fZB2 = 0.0f; // reset filter be zeroing it's state.
	}
}
