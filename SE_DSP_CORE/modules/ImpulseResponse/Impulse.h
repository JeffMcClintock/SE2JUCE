#ifndef IMPULSE_H_INCLUDED
#define IMPULSE_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class Impulse : public MpBase
{
public:
	Impulse( IMpUnknown* host );
	void subProcess( int bufferOffset, int sampleFrames );
	virtual void onSetPins();

private:
	AudioInPin pinTrigger;
	AudioOutPin pinAudioOut;

	bool Triggerstate_;
};

#endif

