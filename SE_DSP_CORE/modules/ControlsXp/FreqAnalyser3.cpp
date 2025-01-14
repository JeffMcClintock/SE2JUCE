/* Copyright (c) 2007-2023 SynthEdit Ltd

Permission to use, copy, modify, and /or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS.IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#define _USE_MATH_DEFINES
#include <math.h>
#include "mp_sdk_audio.h"
#include "../shared/real_fft.h"

SE_DECLARE_INIT_STATIC_FILE(FreqAnalyser3)

using namespace gmpi;

class FreqAnalyser3 final : public MpBase2
{
	BlobOutPin pinCaptureDataA;
	AudioInPin pinSignalin;
	IntInPin pinFFTSize;
	IntInPin pinUpdateRate;

public:
	FreqAnalyser3()
	{
		initializePin( pinCaptureDataA );
		initializePin( pinSignalin );
		initializePin( pinFFTSize );
		initializePin( pinUpdateRate );
	}

	int32_t open() override
	{
		MpBase2::open();	// always call the base class

		setSubProcess(&FreqAnalyser3::subProcess);

		setSleep(false); // disable automatic sleeping.

		return gmpi::MP_OK;
	}

	// wait for waveform to restart.
	void waitAwhile(int sampleFrames)
	{
		timeoutCount_ -= sampleFrames;
		if (timeoutCount_ < 0)
		{
			index_ = 0;
			setSubProcess(&FreqAnalyser3::subProcess);
		}
	}

	void subProcess( int sampleFrames )
	{
		// get pointers to in/output buffers.
		auto signalin = getBuffer(pinSignalin);

		for (int s = sampleFrames; s > 0 ; s--)
		{
			assert(index_ < captureSamples);
			resultsA_[index_++] = *signalin++;

			if (index_ == captureSamples)
			{
				sendResultToGui(0);

				if (index_ >= captureSamples) // done
					return;
			}
		}
	}

	void sendResultToGui(int block_offset)
	{
		if (captureSamples < 100) // latency may mean this pin is initially zero for a short time.
			return;

		auto FFT_SIZE = captureSamples;

		// apply window
		for (int i = 0; i < FFT_SIZE; i++)
		{
			resultsWindowed_[i] = window[i] * resultsA_[i];
		}

		realft(resultsWindowed_.data() - 1, FFT_SIZE, 1);

		// calc power
		float nyquist = resultsWindowed_[1]; // DC & nyquist combined into 1st 2 entries

		// Always divide FFT results b n/2, also compensate for 50% loss in window function.
		const float inverseN = 4.0f / FFT_SIZE;
		for (int i = 1; i < FFT_SIZE / 2; i++)
		{
			const auto& a = resultsWindowed_[2 * i];
			const auto& b = resultsWindowed_[2 * i + 1];
			resultsWindowed_[i] = a * a + b * b; // remainder of dB calculation left to GUI.
		}

		//	resultsWindowed_[0] = fabs(resultsWindowed_[0]) * inverseN * 0.5f; // DC component is divided by 2
		resultsWindowed_[0] = fabsf(resultsWindowed_[0]) * 0.5f; // DC component is divided by 2
		resultsWindowed_[FFT_SIZE / 2] = fabsf(nyquist);

		// GUI does a square root on everything, compensate.
		resultsWindowed_[0] *= resultsWindowed_[0];
		resultsWindowed_[FFT_SIZE / 2] *= resultsWindowed_[FFT_SIZE / 2];

		// FFT produces half the original data.
		const int datasize = (1 + resultsWindowed_.size() / 2) * sizeof(resultsWindowed_[0]);

		// Add an extra member communicating sample-rate to GUI.
		// !! overwrites nyquist value?
		resultsWindowed_[resultsWindowed_.size() / 2] = getSampleRate();

		pinCaptureDataA.setValueRaw(datasize, resultsWindowed_.data());
		pinCaptureDataA.sendPinUpdate(block_offset);

		const int captureRateSamples = (int)getSampleRate() / pinUpdateRate.getValue();
		const int downtime = captureRateSamples - captureSamples;

		if (downtime >= 0)
		{
			timeoutCount_ = downtime;
			setSubProcess(&FreqAnalyser3::waitAwhile);
		}
		else
		{
			const int retainSampleCount = -downtime;
			if (retainSampleCount < captureSamples)
			{
				// we need some overlap to maintain the frame rate
				std::copy(resultsA_.begin() + captureSamples - retainSampleCount, resultsA_.end(), resultsA_.begin());
				index_ = retainSampleCount;
			}
			else
			{
				index_ = 0;
			}
		}

		//timeoutCount_ = (std::max)(100, ((int)getSampleRate() - pinFFTSize.getValue()) / (std::max)(1, pinUpdateRate.getValue()));
		//setSubProcess(&FreqAnalyser3::waitAwhile);

		// if inputs aren't changing, we can sleep.
		if (sleepCount > 0)
		{
			if (--sleepCount == 0)
			{
				setSubProcess(&FreqAnalyser3::subProcessNothing);
				setSleep(true);
			}
		}
	}

	void onSetPins() override
	{
		setSleep(false);

		if (pinFFTSize.isUpdated())
		{
			index_ = 0;
			captureSamples = pinFFTSize;
			resultsA_.assign(captureSamples, 0.0f);
			resultsWindowed_.assign(captureSamples, 0.0f);

			window.resize(captureSamples);

			// create window function
			for (int i = 0; i < captureSamples; i++)
			{
				// sin squared window
				float t = sin(i * ((float)M_PI) / captureSamples);
				window[i] = t * t;
			}
		}

		if (pinSignalin.isStreaming())
		{
			sleepCount = -1; // indicates no sleep.
		}
		else
		{
			// need to send at least 2 captures to ensure flat-line signal captured.
			// Current capture may be half done.
			sleepCount = 2;
		}

		// Avoid resetting capture unless module is actually asleep.
		if (getSubProcess() == &FreqAnalyser3::subProcessNothing)
		{
			index_ = 0;
			setSubProcess(&FreqAnalyser3::subProcess);
		}
	}

protected:
	int index_ = 0;
	int timeoutCount_ = 0;
	int sleepCount = -1;
	std::vector<float> resultsA_;
	std::vector<float> resultsWindowed_;
	std::vector<float> window;
	int captureSamples = 0;
};

namespace
{
	auto r = Register<FreqAnalyser3>::withId(L"SE Freq Analyser3");
}
