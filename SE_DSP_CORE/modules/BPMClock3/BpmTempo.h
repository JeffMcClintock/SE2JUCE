#ifndef BPMTEMPO_H_INCLUDED
#define BPMTEMPO_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class BpmTempo : public MpBase
{
public:
	BpmTempo( IMpUnknown* host );
	void subProcess( int bufferOffset, int sampleFrames );
	virtual void onSetPins();

private:
	FloatInPin pinHostBpm;
	BoolInPin pinHostTransport;
	AudioOutPin pinTransport;
	AudioOutPin pinBpm;
};

#endif
