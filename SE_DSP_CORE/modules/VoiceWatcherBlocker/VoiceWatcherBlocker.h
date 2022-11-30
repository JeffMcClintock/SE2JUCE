#ifndef INVERTERDSP_H_INCLUDED
#define INVERTERDSP_H_INCLUDED

#include "mp_sdk_audio.h"

class VoiceWatcherBlocker : public MpBase
{
public:
	VoiceWatcherBlocker( IMpUnknown* host );

	void subProcess( int bufferOffset, int sampleFrames );
	void onSetPins() override; // one or more pins updated.  Check pin update flags to determin which

private:
	AudioInPin pinSignalIn;
	AudioOutPin pinSignalOut;
};

#endif

