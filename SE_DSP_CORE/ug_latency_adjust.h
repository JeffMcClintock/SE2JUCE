#pragma once
#include <vector>
#include <assert.h>
#include "modules/se_sdk3/mp_sdk_audio.h"
#include "modules/se_sdk2/se_datatypes.h"

class LatencyAdjust : public MpBase2
{
	static const int outputPin = 1;
	std::vector<float> buffer;
	int delayPos;
	int staticCount;

public:
	int latency;

	LatencyAdjust() :
		latency(0)
		, delayPos(0)
	{
		initializePin(pinSignalIn);
		initializePin(pinSignalOut);

		setSubProcess(&LatencyAdjust::subProcess);
	}

	virtual int32_t MP_STDCALL open() override
	{
		// Copy latency setting from master.
		if (latency == 0)
		{
			gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> com_object;
			int32_t r = getHost()->createCloneIterator(com_object.asIMpUnknownPtr());

			gmpi_sdk::mp_shared_ptr<gmpi::IMpCloneIterator> cloneIterator;
			r = com_object->queryInterface(gmpi::MP_IID_CLONE_ITERATOR, cloneIterator.asIMpUnknownPtr());

			gmpi::IMpUnknown* clone;
			cloneIterator->first();
			if (cloneIterator->next(&clone) == gmpi::MP_OK)
			{
				latency = dynamic_cast<LatencyAdjust*>(clone)->latency;
			}
		}

		assert(latency > 0);
		buffer.assign(latency, 0);

		return MpBase2::open();
	}

	virtual void onGraphStart() override
	{
		MpBase2::onGraphStart();

		pinSignalOut.setStreaming(false);
		setSubProcess(&LatencyAdjust::subProcess);

		// Fill buffer with initial value. Effectivly stretching it back in time.
		buffer.assign(buffer.size(), pinSignalIn.getValue(0));
	}

	virtual void onSetPins() override
	{
		host.setPinStreaming(getBlockPosition() + latency, outputPin, pinSignalIn.isStreaming());

		if (!pinSignalIn.isStreaming())
		{
			curSubProcess_ = &MpBase2::subProcessPreSleep;
			resetSleepCounter();
			sleepCount_ += latency;
		}
	}

	void subProcess(int sampleFrames)
	{
		const float* signalIn = getBuffer(pinSignalIn);
		float* signalOut = getBuffer(pinSignalOut);

		for (int s = sampleFrames; s > 0; --s)
		{
			*signalOut = buffer[delayPos];
			buffer[delayPos] = *signalIn;

			// Increment buffer pointers.
			++signalIn;
			++signalOut;
			++delayPos;
			if (delayPos == buffer.size())
			{
				delayPos = 0;
			}
		}
	}

	AudioInPin pinSignalIn;
	AudioOutPin pinSignalOut;
};


