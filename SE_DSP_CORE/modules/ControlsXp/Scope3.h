// Copyright 2006 Jeff McClintock

#ifndef scope3_dsp_H_INCLUDED
#define scope3_dsp_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

#define SCOPE_BUFFER_SIZE 400
#define SCOPE_CHANNELS 2
typedef float ScopeResults[SCOPE_BUFFER_SIZE + 1]; // 4x as big when oversampling. +1 for sample-rate.

class Scope3: public MpBase
{
public:
	Scope3(IMpUnknown* host);

	// overrides
	int32_t MP_STDCALL open() override;

	// methods
	void subProcess(int bufferOffset, int sampleFrames);
	void waitForTrigger1(int bufferOffset, int sampleFrames);
	void waitForTrigger2(int bufferOffset, int sampleFrames);
	void subProcessCruise(int bufferOffset, int sampleFrames);
	void forceTrigger();
	void onSetPins() override;


	virtual AudioInPin* getTriggerPin()
	{
		return &pinSignalA;
	}
	virtual int getTimeOut()
	{
		return (int)getSampleRate() / 3;
	}

protected:
	void sendResultToGui(int block_offset);

	// pins
	BlobOutPin pinSamplesA;
	BlobOutPin pinSamplesB;
	AudioInPin pinSignalA;
	AudioInPin pinSignalB;
	FloatInPin pinVoiceActive;
	BoolOutPin pinPolyDetect;

	int index_;
	int timeoutCount_;
//	int sleepCount;
	ScopeResults resultsA_;
	ScopeResults resultsB_;
//	bool channelActive_[SCOPE_CHANNELS];
	int channelsleepCount_[SCOPE_CHANNELS];
	Scope3** currentVoice_;
	int captureSamples;
};

#endif
