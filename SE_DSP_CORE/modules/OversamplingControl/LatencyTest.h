#ifndef LATENCYTEST_H_INCLUDED
#define LATENCYTEST_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"
#include <vector>

class LatencyTest : public MpBase2
{
public:
	LatencyTest( );
	void subProcess( int sampleFrames );
	virtual void onSetPins() override;

protected:
	AudioInPin pinSignalIn;
	AudioOutPin pinSignalOut;
};

class LatencyTest2 : public LatencyTest
{
	std::vector<float> delayLine;
	int index;

public:
	void subProcess(int sampleFrames)
	{
		const float* signalIn = getBuffer(pinSignalIn);
		float* signalOut = getBuffer(pinSignalOut);

		for (int s = sampleFrames; s > 0; --s)
		{
			if (index >= delayLine.size())
			{
				index = 0;
			}
			*signalOut = delayLine[index];
			delayLine[index++] = *signalIn;

			// Increment buffer pointers.
			++signalIn;
			++signalOut;
		}
	}

	virtual void onSetPins() override
	{
		pinSignalOut.setStreaming(true);

		int latencySamples = 20; // (int)(getSampleRate() * 0.020f); // 20ms

		if (delayLine.size() != latencySamples)
		{
			delayLine.assign(latencySamples, 0.0f);
			index = 0;
		}
		setSubProcess(&LatencyTest2::subProcess);
	}
};

#endif

