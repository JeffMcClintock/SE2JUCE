#ifndef ENVRETRIGGERMODEAUTOSET_H_INCLUDED
#define ENVRETRIGGERMODEAUTOSET_H_INCLUDED

#include "mp_sdk_audio.h"

class EnvRetriggerModeAutoSet : public MpBase
{
public:
	EnvRetriggerModeAutoSet( IMpUnknown* host );
	void subProcess( int bufferOffset, int sampleFrames );
	virtual void onSetPins();

private:
	AudioInPin pinGate;
	FloatInPin pinVoiceReset;
	EnumOutPin pinRetriggerMode;

	int delayCount;
};

#endif

