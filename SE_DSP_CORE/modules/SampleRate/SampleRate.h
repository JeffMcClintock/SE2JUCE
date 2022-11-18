#ifndef SAMPLERATE_H_INCLUDED
#define SAMPLERATE_H_INCLUDED

#include "mp_sdk_audio.h"

class SampleRate : public MpBase
{
public:
	SampleRate( IMpUnknown* host );
	virtual void onGraphStart() override;

private:
	FloatOutPin pinSampleRate;
};

#endif

