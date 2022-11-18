#include "../se_sdk3/mp_sdk_audio.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <cstdint>

/*
PolyBLEP Waveform generator ported from the Jesusonic code by Tale
http://www.taletn.com/reaper/mono_synth/

Permission has been granted to release this port under the WDL/IPlug license:

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

using namespace gmpi;

const double TWO_PI = 2 * M_PI;

template<typename T>
inline T square_number(const T &x) {
	return x * x;
}

// Adapted from "Phaseshaping Oscillator Algorithms for Musical Sound
// Synthesis" by Jari Kleimola, Victor Lazzarini, Joseph Timoney, and Vesa
// Valimaki.
// http://www.acoustics.hut.fi/publications/papers/smc2010-phaseshaping/
inline double blep(double t, double dt) {
	if (t < dt) {
		return -square_number(t / dt - 1);
	}
	else if (t > 1 - dt) {
		return square_number((t - 1) / dt + 1);
	}
	else {
		return 0;
	}
}

// Derived from blep().
inline double blamp(double t, double dt) {
	if (t < dt) {
		t = t / dt - 1;
		return -1 / 3.0 * square_number(t) * t;
	}
	else if (t > 1 - dt) {
		t = (t - 1) / dt + 1;
		return 1 / 3.0 * square_number(t) * t;
	}
	else {
		return 0;
	}
}

template<typename T>
inline int64_t bitwiseOrZero(const T &t) {
	return static_cast<int64_t>(t) | 0;
}

class Oscillator : public MpBase2
{
	AudioInPin pinPitch;
	AudioInPin pinPulseWidth;
	IntInPin pinWaveform;
	AudioInPin pinSync;
	AudioInPin pinPhaseMod;
	AudioOutPin pinAudioOut;
	AudioInPin pinPMDepthdmy;
	BoolInPin pinPolypodPW;
	IntInPin pinOne_Shot;
	BoolInPin pinBypass;
	BoolInPin pinEconomymode;
	IntInPin pinResetMode;
	FloatInPin pinVoiceActive;

	enum Waveform {
		SINE,
		COSINE,
		TRIANGLE,
		SQUARE,
		RECTANGLE,
		SAWTOOTH,
		RAMP,
		MODIFIED_TRIANGLE,
		MODIFIED_SQUARE,
		HALF_WAVE_RECTIFIED_SINE,
		FULL_WAVE_RECTIFIED_SINE,
		TRIANGULAR_PULSE,
		TRAPEZOID_FIXED,
		TRAPEZOID_VARIABLE
	};

	Waveform waveform = SQUARE;
	double sampleRate = 44100;
	double freqInSecondsPerSample;
	double amplitude = 0.5; // Frequency dependent gain [0.0..1.0]
	double pulseWidth = 0.5; // [0.0..1.0]
	double t = 0.0; // The current phase [0.0..1.0) of the oscillator.


public:
	Oscillator()
	{
		initializePin( pinPitch );
		initializePin( pinPulseWidth );
		initializePin( pinWaveform );
		initializePin( pinSync );
		initializePin( pinPhaseMod );
		initializePin( pinAudioOut );
		initializePin( pinPMDepthdmy );
		initializePin( pinPolypodPW );
		initializePin( pinOne_Shot );
		initializePin( pinBypass );
		initializePin( pinEconomymode );
		initializePin( pinResetMode );
		initializePin( pinVoiceActive );

		/*
		PolyBLEP::PolyBLEP(double sampleRate, Waveform waveform, double initialFrequency)
        : waveform(waveform), sampleRate(sampleRate), amplitude(1.0), t(0.0) {
    setSampleRate(sampleRate);
    setFrequency(initialFrequency);
    setWaveform(waveform);
    setPulseWidth(0.5);
}

		*/
		setFrequency(400.0);
	}

	int32_t MP_STDCALL open() override
	{
		setSampleRate(getSampleRate());
		return MpBase2::open();
	}

	const static int maxVolts = 10;
	inline float SampleToVoltage(float s) const
	{
		return s * (float)maxVolts;
	}
	inline float SampleToFrequency(float volts) const
	{
		return 440.f * powf(2.f, SampleToVoltage(volts) - (float)maxVolts * 0.5f);
	}

	void subProcess( int sampleFrames )
	{
		// get pointers to in/output buffers.
		auto pitch = getBuffer(pinPitch);
		auto pulseWidth = getBuffer(pinPulseWidth);
		auto sync = getBuffer(pinSync);
		auto phaseMod = getBuffer(pinPhaseMod);
		auto audioOut = getBuffer(pinAudioOut);
		auto pMDepthdmy = getBuffer(pinPMDepthdmy);

		setFrequency(SampleToFrequency(*pitch));

		for( int s = sampleFrames; s > 0; --s )
		{
			// TODO: Signal processing goes here.
			*audioOut = getAndInc();

			// Increment buffer pointers.
			++pitch;
			++pulseWidth;
			++sync;
			++phaseMod;
			++audioOut;
			++pMDepthdmy;
		}
	}

	void subProcessPitchMod(int sampleFrames)
	{
		// get pointers to in/output buffers.
		auto pitch = getBuffer(pinPitch);
		auto pulseWidth = getBuffer(pinPulseWidth);
		auto sync = getBuffer(pinSync);
		auto phaseMod = getBuffer(pinPhaseMod);
		auto audioOut = getBuffer(pinAudioOut);
		auto pMDepthdmy = getBuffer(pinPMDepthdmy);

		for (int s = sampleFrames; s > 0; --s)
		{
			setFrequency(SampleToFrequency(*pitch));

			// TODO: Signal processing goes here.
			*audioOut = getAndInc();

			// Increment buffer pointers.
			++pitch;
			++pulseWidth;
			++sync;
			++phaseMod;
			++audioOut;
			++pMDepthdmy;
		}
	}

	virtual void onSetPins() override
	{
		// Check which pins are updated.
		if( pinPulseWidth.isStreaming() )
		{
		}
		if( pinWaveform.isUpdated() )
		{
			switch (pinWaveform)
			{
			case 0:
				setWaveform(SINE);
				break;

			case 1:
				setWaveform(SAWTOOTH);
				break;

			case 2:
				setWaveform(RAMP);
				break;

			case 3:
				setWaveform(TRIANGLE);
				break;

			case 4:
				setWaveform(SQUARE);
				break;
			}
		}
		if( pinSync.isStreaming() )
		{
		}
		if( pinPhaseMod.isStreaming() )
		{
		}
		if( pinPMDepthdmy.isStreaming() )
		{
		}
		if( pinPolypodPW.isUpdated() )
		{
		}
		if( pinOne_Shot.isUpdated() )
		{
		}
		if( pinBypass.isUpdated() )
		{
		}
		if( pinEconomymode.isUpdated() )
		{
		}
		if( pinResetMode.isUpdated() )
		{
		}
		if( pinVoiceActive.isUpdated() )
		{
		}

		// Set state of output audio pins.
		pinAudioOut.setStreaming(true);

		if (pinPitch.isStreaming())
		{
			// Set processing method.
			setSubProcess(&Oscillator::subProcessPitchMod);
		}
		else
		{
			setSubProcess(&Oscillator::subProcess);
		}
		
		// Set sleep mode (optional).
		// setSleep(false);
	}

	void setdt(double time) {
		freqInSecondsPerSample = time;
	}

	void setFrequency(double freqInHz) {
		setdt(freqInHz / sampleRate);
	}

	void setSampleRate(double sampleRate) {
		const double freqInHz = getFreqInHz();
		this->sampleRate = sampleRate;
		setFrequency(freqInHz);
	}

	double getFreqInHz() const {
		return freqInSecondsPerSample * sampleRate;
	}

	void setPulseWidth(double pulseWidth) {
		this->pulseWidth = pulseWidth;
	}

	void sync(double phase) {
		t = phase;
		if (t >= 0) {
			t -= bitwiseOrZero(t);
		}
		else {
			t += 1 - bitwiseOrZero(t);
		}
	}

	void setWaveform(Waveform waveform) {
		this->waveform = waveform;
	}

	double get() const {
		if (getFreqInHz() >= sampleRate / 4) {
			return sin();
		}
		else switch (waveform) {
		case SINE:
			return sin();
		case COSINE:
			return cos();
		case TRIANGLE:
			return tri();
		case SQUARE:
			return sqr();
		case RECTANGLE:
			return rect();
		case SAWTOOTH:
			return saw();
		case RAMP:
			return ramp();
		case MODIFIED_TRIANGLE:
			return tri2();
		case MODIFIED_SQUARE:
			return sqr2();
		case HALF_WAVE_RECTIFIED_SINE:
			return half();
		case FULL_WAVE_RECTIFIED_SINE:
			return full();
		case TRIANGULAR_PULSE:
			return trip();
		case TRAPEZOID_FIXED:
			return trap();
		case TRAPEZOID_VARIABLE:
			return trap2();
		default:
			return 0.0;
		}
	}

	void inc() {
		t += freqInSecondsPerSample;
		t -= bitwiseOrZero(t);
	}

	double getAndInc() {
		const double sample = get();
		inc();
		return sample;
	}

	double sin() const {
		return amplitude * std::sin(TWO_PI * t);
	}

	double cos() const {
		return amplitude * std::cos(TWO_PI * t);
	}

	double half() const {
		double t2 = t + 0.5;
		t2 -= bitwiseOrZero(t2);

		double y = (t < 0.5 ? 2 * std::sin(TWO_PI * t) - 2 / M_PI : -2 / M_PI);
		y += TWO_PI * freqInSecondsPerSample * (blamp(t, freqInSecondsPerSample) + blamp(t2, freqInSecondsPerSample));

		return amplitude * y;
	}

	double full() const {
		double _t = this->t + 0.25;
		_t -= bitwiseOrZero(_t);

		double y = 2 * std::sin(M_PI * _t) - 4 / M_PI;
		y += TWO_PI * freqInSecondsPerSample * blamp(_t, freqInSecondsPerSample);

		return amplitude * y;
	}

	double tri() const {
		double t1 = t + 0.25;
		t1 -= bitwiseOrZero(t1);

		double t2 = t + 0.75;
		t2 -= bitwiseOrZero(t2);

		double y = t * 4;

		if (y >= 3) {
			y -= 4;
		}
		else if (y > 1) {
			y = 2 - y;
		}

		y += 4 * freqInSecondsPerSample * (blamp(t1, freqInSecondsPerSample) - blamp(t2, freqInSecondsPerSample));

		return amplitude * y;
	}

	double tri2() const {
		double pulseWidth = std::fmax(0.0001, std::fmin(0.9999, this->pulseWidth));

		double t1 = t + 0.5 * pulseWidth;
		t1 -= bitwiseOrZero(t1);

		double t2 = t + 1 - 0.5 * pulseWidth;
		t2 -= bitwiseOrZero(t2);

		double y = t * 2;

		if (y >= 2 - pulseWidth) {
			y = (y - 2) / pulseWidth;
		}
		else if (y >= pulseWidth) {
			y = 1 - (y - pulseWidth) / (1 - pulseWidth);
		}
		else {
			y /= pulseWidth;
		}

		y += freqInSecondsPerSample / (pulseWidth - pulseWidth * pulseWidth) * (blamp(t1, freqInSecondsPerSample) - blamp(t2, freqInSecondsPerSample));

		return amplitude * y;
	}

	double trip() const {
		double t1 = t + 0.75 + 0.5 * pulseWidth;
		t1 -= bitwiseOrZero(t1);

		double y;
		if (t1 >= pulseWidth) {
			y = -pulseWidth;
		}
		else {
			y = 4 * t1;
			y = (y >= 2 * pulseWidth ? 4 - y / pulseWidth - pulseWidth : y / pulseWidth - pulseWidth);
		}

		if (pulseWidth > 0) {
			double t2 = t1 + 1 - 0.5 * pulseWidth;
			t2 -= bitwiseOrZero(t2);

			double t3 = t1 + 1 - pulseWidth;
			t3 -= bitwiseOrZero(t3);
			y += 2 * freqInSecondsPerSample / pulseWidth * (blamp(t1, freqInSecondsPerSample) - 2 * blamp(t2, freqInSecondsPerSample) + blamp(t3, freqInSecondsPerSample));
		}
		return amplitude * y;
	}

	double trap() const {
		double y = 4 * t;
		if (y >= 3) {
			y -= 4;
		}
		else if (y > 1) {
			y = 2 - y;
		}
		y = std::fmax(-1, std::fmin(1, 2 * y));

		double t1 = t + 0.125;
		t1 -= bitwiseOrZero(t1);

		double t2 = t1 + 0.5;
		t2 -= bitwiseOrZero(t2);

		// Triangle #1
		y += 4 * freqInSecondsPerSample * (blamp(t1, freqInSecondsPerSample) - blamp(t2, freqInSecondsPerSample));

		t1 = t + 0.375;
		t1 -= bitwiseOrZero(t1);

		t2 = t1 + 0.5;
		t2 -= bitwiseOrZero(t2);

		// Triangle #2
		y += 4 * freqInSecondsPerSample * (blamp(t1, freqInSecondsPerSample) - blamp(t2, freqInSecondsPerSample));

		return amplitude * y;
	}

	double trap2() const {
		double pulseWidth = std::fmin(0.9999, this->pulseWidth);
		double scale = 1 / (1 - pulseWidth);

		double y = 4 * t;
		if (y >= 3) {
			y -= 4;
		}
		else if (y > 1) {
			y = 2 - y;
		}
		y = std::fmax(-1, std::fmin(1, scale * y));

		double t1 = t + 0.25 - 0.25 * pulseWidth;
		t1 -= bitwiseOrZero(t1);

		double t2 = t1 + 0.5;
		t2 -= bitwiseOrZero(t2);

		// Triangle #1
		y += scale * 2 * freqInSecondsPerSample * (blamp(t1, freqInSecondsPerSample) - blamp(t2, freqInSecondsPerSample));

		t1 = t + 0.25 + 0.25 * pulseWidth;
		t1 -= bitwiseOrZero(t1);

		t2 = t1 + 0.5;
		t2 -= bitwiseOrZero(t2);

		// Triangle #2
		y += scale * 2 * freqInSecondsPerSample * (blamp(t1, freqInSecondsPerSample) - blamp(t2, freqInSecondsPerSample));

		return amplitude * y;
	}

	double sqr() const {
		double t2 = t + 0.5;
		t2 -= bitwiseOrZero(t2);

		double y = t < 0.5 ? 1 : -1;
		y += blep(t, freqInSecondsPerSample) - blep(t2, freqInSecondsPerSample);

		return amplitude * y;
	}

	double sqr2() const {
		double t1 = t + 0.875 + 0.25 * (pulseWidth - 0.5);
		t1 -= bitwiseOrZero(t1);

		double t2 = t + 0.375 + 0.25 * (pulseWidth - 0.5);
		t2 -= bitwiseOrZero(t2);

		// Square #1
		double y = t1 < 0.5 ? 1 : -1;

		y += blep(t1, freqInSecondsPerSample) - blep(t2, freqInSecondsPerSample);

		t1 += 0.5 * (1 - pulseWidth);
		t1 -= bitwiseOrZero(t1);

		t2 += 0.5 * (1 - pulseWidth);
		t2 -= bitwiseOrZero(t2);

		// Square #2
		y += t1 < 0.5 ? 1 : -1;

		y += blep(t1, freqInSecondsPerSample) - blep(t2, freqInSecondsPerSample);

		return amplitude * 0.5 * y;
	}

	double rect() const {
		double t2 = t + 1 - pulseWidth;
		t2 -= bitwiseOrZero(t2);

		double y = -2 * pulseWidth;
		if (t < pulseWidth) {
			y += 2;
		}

		y += blep(t, freqInSecondsPerSample) - blep(t2, freqInSecondsPerSample);

		return amplitude * y;
	}

	double saw() const {
		double _t = t + 0.5;
		_t -= bitwiseOrZero(_t);

		double y = 2 * _t - 1;
		y -= blep(_t, freqInSecondsPerSample);

		return amplitude * y;
	}

	double ramp() const {
		double _t = t;
		_t -= bitwiseOrZero(_t);

		double y = 1 - 2 * _t;
		y += blep(_t, freqInSecondsPerSample);

		return amplitude * y;
	}
};

namespace
{
	auto r = Register<Oscillator>::withId(L"SE Oscillator");
}
