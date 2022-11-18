#include "./KorgFilter.h"
#include <limits.h>

REGISTER_PLUGIN2 ( KorgFilter, L"SE Korg Filter" );

inline static unsigned int FrequencyToIntIncrement(float sampleRate, double freq_hz)
{
	double temp_float = (UINT_MAX + 1.f) * freq_hz / sampleRate + 0.5f;
	return (unsigned int)temp_float;
}

KorgFilter::KorgFilter()
{
	// Register pins.
	initializePin(0, pinSignal);
	initializePin(1, pinPitch);
	initializePin(2, pinResonance);
	initializePin(3, pinSaturation);
	initializePin(4, pinMode);
	initializePin(5, pinSaturationEnable);
	initializePin(6, pinOutput);
}

int32_t KorgFilter::open()
{
	m_LPF1.m_uFilterType = LPF1;
	m_LPF2.m_uFilterType = LPF1;
	m_HPF1.m_uFilterType = HPF1;

	// flush everything
	m_LPF1.reset();
	m_LPF2.reset();
	m_HPF1.reset();

	// 20kHz is about 10.5 Volts. 1Hz is about -3.7 volts. 0.01Hz = -10V
	// -4 -> 11 Volts should cover most posibilities. 15V Range. 12 entries per volt = 180 entries.
	const int entriesPerOctave = 12;
	const int extraEntriesAtStart = 1; // for interpolator.
	const int extraEntriesAtEnd = 3; // for interpolator.
	const int pitchTableSize = extraEntriesAtStart + extraEntriesAtEnd + (pitchTableHiVolts - pitchTableLowVolts) * entriesPerOctave;
	int size = (pitchTableSize)* sizeof(float);
	int32_t needInitialize;
	getHost()->allocateSharedMemory(L"KorgFilter:Pitch", (void**)&pitchTable, getSampleRate(), size, needInitialize);

	safeMaxHz = 0.4782f * getSampleRate(); // stability limit.

	if (needInitialize)
	{
		float T = 1.0f / getSampleRate();
		for (int i = 0; i < pitchTableSize; ++i)
		{
			float pitch = pitchTableLowVolts + (i - extraEntriesAtStart) / (float)entriesPerOctave;
			float hz = 440.f * powf(2.f, pitch - 5.f);
			hz = (std::min)(hz, safeMaxHz);

			double wd = 2 * M_PI * hz;
			double T = 1 / (double)getSampleRate();
			double wa = (2 / T)*tan(wd*T / 2);
			double g = wa*T / 2;

			pitchTable[i] = (float) g;
			// _RPT3(_CRT_WARN, "%3d %f %f\n", i, pitch, pitchTable[i] );
		}
	}

	pitchTable += extraEntriesAtStart; // Shift apparent start of table to entry #1, so we can access table[-1] without hassle.
	pitchTable -= pitchTableLowVolts * entriesPerOctave; // Shift apparent start of table Volts = 0.0.

	// init max freq lookup table (given reson, lookup max freq filter can handle)
	size = 514;
	getHost()->allocateSharedMemory(L"KorgFilter:StabilityLimit", (void**)&stabilityTable, getSampleRate(), size * sizeof(float), needInitialize);
	if (needInitialize)
	{
		for (int j = 0; j < size; j++)
		{
			// orig, too harsh			max_freq_lookup->SetValue( j,VoltageToSample( 0.000025 * j*j + 0.009 * j + 8.2378) );
			float temp = (0.000025f * j*j + 0.009f * j + 7.8f);
			stabilityTable[j] = temp * 0.1f;
		}
	}

	return FilterBase::open();
}

void KorgFilter::onSetPins()
{
	// Check which pins are updated.
	if (pinPitch.isUpdated() && !pinPitch.isStreaming())
	{
		PitchFixed::CalcInitial(pitchTable, pinPitch, coef_g);
		ResonanceFixed::CalcInitial(pinResonance, pinPitch, coef_R); // Resonance stability depends on pitch.
	}
	else
	{
		if (pinResonance.isUpdated() && !pinResonance.isStreaming())
		{
			ResonanceFixed::CalcInitial(pinResonance, pinPitch, coef_R);
		}
	}

	// Set state of output audio pins.
	pinOutput.setStreaming(true);

	// Set processing method.

	typedef void (KorgFilter::* MyProcess_ptr)(int sampleFrames);
	#define PROCESS_PTR( pitch, mode ) &KorgFilter::subProcessZdf< pitch, mode >

	// NOTE: Resonance limits depend on pitch, so when pitch modulated, need to calc resonance too.
	const static MyProcess_ptr ProcessSelection[2][2] =
	{
		PROCESS_PTR(PitchFixed, LowPass),
		PROCESS_PTR(PitchFixed, HiPass),
		PROCESS_PTR(PitchChanging, LowPass),
		PROCESS_PTR(PitchChanging, HiPass),
	};

	// If reson is moduled, have to treat freq as if it's modulated (as maximum stabel freq is therefore modulated).
	setSubProcess(static_cast <SubProcess_ptr2> (ProcessSelection[pinPitch.isStreaming() || pinResonance.isStreaming()][pinMode]));

	m_dSaturation = 1.0f + 0.1f * pinSaturation * (2.0f - 1.0f); // ? Saturation (1.0 - 2.0)
	m_uNonLinearProcessing = pinSaturationEnable;

	if (pinPitch.isUpdated() || pinResonance.isUpdated())
	{
		UpdateFiltersLP(pinPitch, pinResonance);
	}

	initSettling();
}
