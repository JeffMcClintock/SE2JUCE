#include "./Rms.h"

REGISTER_PLUGIN2 ( Rms, L"SE RMS" );

void Rms::initializePins()
{
	initializePin(pinSignal);
	initializePin(pinPitch);
	initializePin(pinOutput);
}

/*
int32_t Rms::open()
{
	// 20kHz is about 10.5 Volts. 1Hz is about -3.7 volts. 0.01Hz = -10V
	// -4 -> 11 Volts should cover most posibilities. 15V Range. 12 entries per volt = 180 entries.
	const int tableSize = extraEntriesAtStart + extraEntriesAtEnd + tableEntries;
	int size = (tableSize) * sizeof(float);
	int32_t needInitialize;
	getHost()->allocateSharedMemory(L"SE RMS:sqrt", (void**)&sqrtTable, getSampleRate(), size, needInitialize);

	if (needInitialize)
	{
		for (int i = extraEntriesAtStart; i < tableSize ; ++i)
		{
			// cover the range 0 - 20V
			double v = 2.0 * (i - extraEntriesAtStart) / (double)tableEntries;
			sqrtTable[i] = sqrtf(v);
			// _RPT3(_CRT_WARN, "%3d %f %f\n", i, pitch, pitchTable[i] );
		}

		sqrtTable[0] = -sqrtTable[2]; // extrapolate first entry
	}

	sqrtTable += extraEntriesAtStart; // Shift apparent start of table to entry #1, so we can access table[-1] without hassle.

	return SvFilter2::open();
}
*/

void Rms::onSetPins()
{
//	if (pinResonance.isUpdated() && !pinResonance.isStreaming())
	{
		const float fixedResonance = 0.5f;
		ResonanceFixed::CalcInitial(stabilityTable, fixedResonance, maxStableF1, quality);
	}

	if (pinPitch.isUpdated() && !pinPitch.isStreaming())
	{
		// Pitch is same as normal filter divided by "pitchAdjustment".
		PitchFixed::CalcInitial(pitchTable, pinPitch * pitchAdjustment, maxStableF1, f1);
	}

	ChoseProcessMethod();

	initSettling();
}

void Rms::ChoseProcessMethod()
{
	// Set state of output audio pins.
	pinOutput.setStreaming(true);

	if (pinPitch.isStreaming())
	{
		setSubProcess(&Rms::subProcess< PitchChanging, ResonanceFixed, FilterModeLowPass, 1 >);
	}
	else
	{
		setSubProcess(&Rms::subProcess< PitchFixed, ResonanceFixed, FilterModeLowPass, 1 >);
	}
}