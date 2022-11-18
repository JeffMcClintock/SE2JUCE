#ifndef BPMCLOCK3_H_INCLUDED
#define BPMCLOCK3_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class BpmClock3 : public MpBase
{
public:
	BpmClock3( IMpUnknown* host );
	void subProcess2( int bufferOffset, int sampleFrames );
	virtual void onSetPins();
	static double CalcAccumulatorError(float HostSongPosition, double accumulator, int pulseDivision, float hostBarStart, int numerator, int denominator);

private:
	void CalcIncrement( int pulseDivide );

	double accumulator;
	double increment;

	float pulseOutVal;
	int static_count;
	bool resyncInProgress;

	FloatInPin pinHostBpm;
	FloatInPin pinHostSongPosition;
	FloatInPin pinHostBarStart;
	IntInPin pinPulseDivide;
	IntInPin pinNumerator;
	IntInPin pinDenominator;
	AudioOutPin pinPulseOut;
	BoolInPin pinHostTransport;
    double ppqPos;
    double incrementPerSample;
};

#endif

