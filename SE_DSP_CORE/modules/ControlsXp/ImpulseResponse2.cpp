#include "../se_sdk3/mp_sdk_audio.h"
#include "../shared/real_fft.h"
#define _USE_MATH_DEFINES
#include <math.h>

using namespace gmpi;

SE_DECLARE_INIT_STATIC_FILE(ImpulseResponse2);

class ImpulseResponse2 : public MpBase2
{
	AudioInPin pinImpulsein;
	AudioInPin pinSignalin;
	IntInPin pinFreqScale;
	BlobOutPin pinResults;
	FloatOutPin pinSampleRateToGui;

	static const int FFT_SIZE = 4096;

	float results[FFT_SIZE];
	float buffer[FFT_SIZE];

	float* input_ptr;
	int m_idx = 0;

public:
	ImpulseResponse2()
	{
		// Register pins.
		initializePin(0, pinImpulsein);
		initializePin(1, pinSignalin);
		initializePin(2, pinFreqScale);
		initializePin(3, pinResults);
		initializePin(4, pinSampleRateToGui);

		memset(results, 0, sizeof(results));
		// todo? SetFlag(UGF_POLYPHONIC_AGREGATOR|UGF_VOICE_MON_IGNORE);
	}

	// do nothing while UI updates
	void subProcessIdle(int sampleFrames)
	{
		auto impulse = getBuffer(pinImpulsein);

		// Time till next impulse
		int to_pos = 0;
		while (*impulse < 1.0f && to_pos < sampleFrames)
		{
			++to_pos;
			++impulse;
		}

		if (to_pos != sampleFrames)
		{
			TempBlockPositionSetter x(this, getBlockPosition() + to_pos);

			subProcess(sampleFrames - to_pos);
			setSubProcess(&ImpulseResponse2::subProcess);
		}
	}

	void subProcess(int sampleFrames)
	{
		// get pointers to in/output buffers.
		auto signal = getBuffer(pinSignalin);

		int count = (std::min)(sampleFrames, FFT_SIZE - m_idx);

		assert(m_idx >= 0 && m_idx < FFT_SIZE);

		while (count-- > 0)
		{
			buffer[m_idx++] = *signal++;
		}

		if (m_idx >= FFT_SIZE)
		{
			printResult();
			SET_PROCESS2(&ImpulseResponse2::subProcessIdle);
		}
	}

	void onSetPins()
	{
		// Set processing method.
		SET_PROCESS2(&ImpulseResponse2::subProcess);
	}

	void printResult()
	{
		m_idx = 0;

		pinSampleRateToGui.setValue(getSampleRate(), 0);

		if (pinFreqScale == 0) // Impulse Response
		{
			// send an odd number of samples to signal it's the raw impulse.
			pinResults.setValueRaw(sizeof(buffer[0]) * FFT_SIZE / 2 - 1, &buffer);
			pinResults.sendPinUpdate(0);
			return;
		}

		// no window (square window).
		// Calculate FFT.
		realft2(buffer, FFT_SIZE, 1);

		// calc power. Get magnitude of real and imaginary vectors.
		float nyquist = buffer[1]; // DC & nyquist combined into 1st 2 entries
		for (int i = 1; i < FFT_SIZE / 2; i++)
		{
			float power = buffer[2 * i] * buffer[2 * i] + buffer[2 * i + 1] * buffer[2 * i + 1];
			// square root
			power = sqrtf(power);
			results[i] = power;
		}

		results[0] = fabs(buffer[0]) / 2.f; // DC component is divided by 2
		results[FFT_SIZE / 2] = fabs(nyquist);

		// Send to GUI.
		pinResults.setValueRaw(sizeof(results[0]) * FFT_SIZE / 2, &results);
		pinResults.sendPinUpdate(0);
	}
};

namespace
{
	auto r = Register<ImpulseResponse2>::withId(L"SE Impulse Response2");
}
