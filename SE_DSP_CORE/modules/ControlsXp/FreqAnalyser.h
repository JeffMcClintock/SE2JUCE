// Copyright 2006 Jeff McClintock

#ifndef scope3_dsp_H_INCLUDED
#define scope3_dsp_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"
#include <vector>

class FreqAnalyser: public MpBase2
{
public:
	FreqAnalyser();

	// overrides
	int32_t MP_STDCALL open() override;

	// methods
	void subProcess(int sampleFrames);
	void waitAwhile(int sampleFrames);
	void onSetPins() override;

	virtual int getTimeOut()
	{
		return (int)getSampleRate() / 3;
	}

protected:
	void sendResultToGui(int block_offset);

	// pins
	BlobOutPin pinSamplesA;
	AudioInPin pinSignalA;
	IntInPin pinCaptureSize;
	IntInPin pinUpdateRate;

	int index_;
	int timeoutCount_;
	int sleepCount;
	std::vector<float> resultsA_;
	std::vector<float> window;
	int captureSamples;
};

#endif
