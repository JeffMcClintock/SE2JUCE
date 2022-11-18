#include "./OscillatorNaive.h"

REGISTER_PLUGIN2 ( OscillatorNaive, L"SE Oscillator (naive)" );
SE_DECLARE_INIT_STATIC_FILE(OscillatorNaive);

#define MAX_VOLTS ( 10.0 )
#define FSampleToVoltage(s) ( (s) * MAX_VOLTS)
#define FSampleToFrequency(volts) ( 440.0 * pow(2.0, FSampleToVoltage(volts) - MAX_VOLTS * 0.5 ) )


OscillatorNaive::OscillatorNaive( ) :
	accumulator(5)
	,pitchTable(0)
	,prevPhase(0)
{
	// Register pins.
	initializePin( pinPitch );
	initializePin( pinPulseWidth );
	initializePin( pinWaveform );
	initializePin( pinSync );
	initializePin( pinPhaseMod );
	initializePin( pinAudioOut );
}

int32_t OscillatorNaive::open()
{
	// 20kHz is about 10.5 Volts. 1Hz is about -3.7 volts. 0.01Hz = -10V
	// -4 -> 11 Volts should cover most posibilities. 15V Range. 12 entries per volt = 180 entries.
	const int extraEntriesAtStart = 1; // for interpolator.
	const int extraEntriesAtEnd = 3; // for interpolator.
	const int pitchTableSize = extraEntriesAtStart + extraEntriesAtEnd + (OscPitchChanging::pitchTableHiVolts - OscPitchChanging::pitchTableLowVolts) * 12;
	constexpr float oneSemitone = 1.0f / 12.0f;
	int size = (pitchTableSize) * sizeof(pitchTable[0]);
	int32_t needInitialize;
	getHost()->allocateSharedMemory(L"SE:OscillatorNaive:Pitch", (void**)&pitchTable, getSampleRate(), size, needInitialize);

	if (needInitialize)
	{
		double overSampleRate = 1.0 / getSampleRate();
		for (int i = 0; i < pitchTableSize; ++i)
		{
			double pitch = (OscPitchChanging::pitchTableLowVolts + (i - extraEntriesAtStart) * oneSemitone) * 0.1f;
			double hz = FSampleToFrequency(pitch);

			pitchTable[i] = hz * overSampleRate;
			// _RPT3(_CRT_WARN, "%3d %f %f\n", i, pitch, pitchTable[i] );
		}
	}

	pitchTable += extraEntriesAtStart; // Shift apparent start of table to entry #1, so we can access table[-1] without hassle.

	return MpBase2::open();
}

typedef void (OscillatorNaive::* OscProcess_ptr)(int sampleFrames);

#define TPB( wave, pitch, phase, sync ) (&OscillatorNaive::subProcess< OscillatorNaive::wave, OscillatorNaive::pitch, OscillatorNaive::phase, OscillatorNaive::sync > )

const OscProcess_ptr ProcessSelection[4][2][2][2] =
{
	TPB(WaveShapeSine, OscPitchFixed, NotModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeSine, OscPitchFixed, NotModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapeSine, OscPitchFixed, ModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeSine, OscPitchFixed, ModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapeSine, OscPitchChanging, NotModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeSine, OscPitchChanging, NotModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapeSine, OscPitchChanging, ModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeSine, OscPitchChanging, ModulatedPinPolicy, ModulatedPinPolicy),

	TPB(WaveShapeSaw, OscPitchFixed   , NotModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeSaw, OscPitchFixed   , NotModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapeSaw, OscPitchFixed   , ModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeSaw, OscPitchFixed   , ModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapeSaw, OscPitchChanging, NotModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeSaw, OscPitchChanging, NotModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapeSaw, OscPitchChanging, ModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeSaw, OscPitchChanging, ModulatedPinPolicy, ModulatedPinPolicy),

	TPB(WaveShapePulse, OscPitchFixed   , NotModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapePulse, OscPitchFixed   , NotModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapePulse, OscPitchFixed   , ModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapePulse, OscPitchFixed   , ModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapePulse, OscPitchChanging, NotModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapePulse, OscPitchChanging, NotModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapePulse, OscPitchChanging, ModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapePulse, OscPitchChanging, ModulatedPinPolicy, ModulatedPinPolicy),

	TPB(WaveShapeTriangle, OscPitchFixed   , NotModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeTriangle, OscPitchFixed   , NotModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapeTriangle, OscPitchFixed   , ModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeTriangle, OscPitchFixed   , ModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapeTriangle, OscPitchChanging, NotModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeTriangle, OscPitchChanging, NotModulatedPinPolicy, ModulatedPinPolicy),
	TPB(WaveShapeTriangle, OscPitchChanging, ModulatedPinPolicy, NotModulatedPinPolicy),
	TPB(WaveShapeTriangle, OscPitchChanging, ModulatedPinPolicy, ModulatedPinPolicy),
};

void OscillatorNaive::onSetPins()
{
	if (pinPitch.isUpdated() && !pinPitch.isStreaming())
	{
		float pitch = pinPitch;
		OscPitchChanging::Calculate(pitchTable, &pitch, increment);
	}

	if (pinPhaseMod.isUpdated() && !pinPhaseMod.isStreaming())
	{
		const auto phaseMod = pinPhaseMod.getValue();
		const double delataPhase = prevPhase - phaseMod;
		prevPhase = phaseMod;

		accumulator += delataPhase * 0.5f;
		assert(accumulator > 0.0f);
	}

	// one_off change in sync
	if (pinSync.isUpdated() && !pinSync.isStreaming())
	{
		const auto sync = pinSync.getValue();
		if ((sync > 0.0f) != syncState)
		{
			syncState = sync > 0.0f;
			if (syncState)
			{
				accumulator = 5.0 - pinPhaseMod.getValue() * 0.5f;
			}
		}
	}
	 
	// Set state of output audio pins.
	pinAudioOut.setStreaming(true);

	setSubProcess(static_cast <SubProcess_ptr2> (ProcessSelection[pinWaveform][pinPitch.isStreaming()][pinPhaseMod.isStreaming()][pinSync.isStreaming()]));
}

