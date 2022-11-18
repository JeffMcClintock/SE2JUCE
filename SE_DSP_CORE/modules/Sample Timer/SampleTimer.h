#ifndef SAMPLETIMER_H_INCLUDED
#define SAMPLETIMER_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class SampleTimer : public MpBase2
{
	float outValue_;
	int timer_;

public:
	SampleTimer( );
	void subProcess( int sampleFrames );
	virtual void onSetPins(void);

private:
	IntInPin pinTimeIn;
	AudioOutPin pinSignalOut;
};

#endif

