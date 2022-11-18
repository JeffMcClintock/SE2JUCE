#ifndef PITCHDETECTOR_H_INCLUDED
#define PITCHDETECTOR_H_INCLUDED

#include "mp_sdk_audio.h"
#include <vector>

class MedianFilter
{
	float median;
	float average;

public:
	MedianFilter() : median ( 0.0f ), average ( 0.0f ) {};

// for each sample
	void process( float sample )
	{
//		const float smoothing = 0.03f;
		const float smoothing = 0.03f;

		average += ( abs(sample) - average ) * 0.1f; // rough running average magnitude.
//		float step = min( average * smoothing, abs( sample - median ) );
//		median += (float) _copysign( step, sample - median );

		//float step = (sample - median) * smoothing;
		//median += step;
		median += (float) _copysign( average * smoothing, sample - median );
	};

	float output()
	{
		return median;
	};
};

// Audio 1-pole filter.
struct filter
{
	filter() : filter_y1n(0.0f){};
	float processLoPass( float input )
	{
		// 1 pole low pass.
		filter_y1n = input + filter_l * ( filter_y1n - input );
		return filter_y1n;
	}
	float processHiPass( float input )
	{
		// 1 pole high pass.
		filter_y1n = input + filter_l * ( filter_y1n - input );
		return input - filter_y1n;
	}

	float filter_y1n;
	float filter_l;

	void setHz( float frequencyHz, float sampleRate );
};
class PitchDetector : public MpBase
{
	static const int fftSize = 1024;
	std::vector<float> buffer;
	int bufferIdx;
	int frameNumber;
	MedianFilter medianFilter;
	filter hiPassFilter_;

public:
	PitchDetector( IMpUnknown* host );
	virtual int32_t MP_STDCALL open();
	void subProcess( int bufferOffset, int sampleFrames );
	void subProcess2( int bufferOffset, int sampleFrames );
	virtual void onSetPins(void);

private:
	void calcAveragePitch( void );

	AudioInPin pinInput;
	AudioInPin pinPulseWidth;
	IntInPin pinScale;
	AudioOutPin pinAudioOut;

	float output_;
	float PeakDetectorDecay_;
	float currentPeak_;
	float currentNegativePeak_;
//	float prevInput_;
//	float prevPeak_;

	// DC filter
	float filter_y1n;
	float filter_l;

	bool detectorOut_;
	time_t time_;
	time_t previousPeakTime_;
	static const int outputMemorySize = 64;
	static const int GuiUpdateRateHz = 20;
//	float outputMemory[outputMemorySize];
	float outputMemoryPeriod[outputMemorySize];
	int outputMemoryIdx;
	std::vector<float> PeriodSamples;
	int updateCounter_;
};

#endif

