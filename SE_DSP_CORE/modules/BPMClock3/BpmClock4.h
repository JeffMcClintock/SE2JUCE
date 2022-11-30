#ifndef BPMCLOCK3_H_INCLUDED
#define BPMCLOCK3_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class BpmClock4 : public MpBase2
{
public:
	BpmClock4();
	void subProcess2(int sampleFrames);
	void onSetPins() override;
	double CalcAccumulatorError(double HostSongPosition, double accumulator, float overmultiplier, float hostBarStart) const;

	const int PPQN = 384;

protected:
	void CalcIncrement(float overmultiplier);

	float CalculateMultiplier(int PulseDivisionSetting, int numerator, int denominator)
	{
		// SE enums only 16-bit, correct for overflow.
		if (PulseDivisionSetting < 0)
			PulseDivisionSetting += 0x10000;

		if (PulseDivisionSetting > PPQN) // Sync specified in bars?
		{
			numerator = (std::max)(numerator, 1);
			denominator = (std::max)(denominator, 1);
			// Take time signature into account.
			return (PPQN * denominator) / (2.0f * PulseDivisionSetting * (float)numerator);
		}
		else
		{
			// Sync specified in quarter notes.
			return ((float)PPQN) / (2.0f * PulseDivisionSetting);
		}
	}

	double accumulator;
	double increment;

	float pulseOutVal;
	int static_count;
//	bool resyncInProgress;

	FloatInPin pinSwing;
	FloatInPin pinPulseWidth;
	FloatInPin pinHostBpm;
	FloatInPin pinHostSongPosition;
	FloatInPin pinHostBarStart;
	IntInPin pinPulseDivide;
	IntInPin pinNumerator;
	IntInPin pinDenominator;
	AudioOutPin pinPulseOut;
	BoolInPin pinHostTransport;

//	double quarterNotePerBar;
	double qnPos;
	double qnPosIncPerSample;
	double PulseWidthAmnt;
	double swingAmnt;
	int samplesSinceTransition;
	int syncBars;
	int quarterNotesPerBar;

	int currentState;
	double nextTransition;
};

#endif

