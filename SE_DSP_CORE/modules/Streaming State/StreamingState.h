#ifndef STREAMINGSTATE_H_INCLUDED
#define STREAMINGSTATE_H_INCLUDED

#include "mp_sdk_audio.h"
#include "../se_sdk3/smart_audio_pin.h" 

class StreamingState : public MpBase
{
public:
	StreamingState( IMpUnknown* host );
	void subProcess( int bufferOffset, int sampleFrames );
	virtual void onSetPins();

private:
	AudioInPin pinSignalIn;
	SmartAudioPin pinSignalOut;
	float output_;
};

#endif

