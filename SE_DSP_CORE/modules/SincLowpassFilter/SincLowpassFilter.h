#ifndef SINCLOWPASSFILTER_H_INCLUDED
#define SINCLOWPASSFILTER_H_INCLUDED

#include "mp_sdk_audio.h"
#include "SincFilter.h"

class SincLowpassFilter : public MpBase
{
public:
	SincLowpassFilter( IMpUnknown* host );
	void subProcess( int bufferOffset, int sampleFrames );
	virtual void onSetPins(void);
	virtual int32_t MP_STDCALL open();

private:
	static const int numCoefs = 168;
	static const int maxCoefs = 600;

	SincFilter filter_;
	SincFilterCoefs coefs_;

	AudioInPin pinSignal;
	AudioInPin pinPitch;
	IntInPin pinTaps;
	AudioOutPin pinOutput;
	AudioOutPin pinOutput2;
	float taps[maxCoefs];
	float* hist;
	int histIdx;
};

#endif

