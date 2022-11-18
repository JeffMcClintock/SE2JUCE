#ifndef db_to_voltage_H_INCLUDED
#define db_to_voltage_H_INCLUDED

#include "mp_sdk_audio.h"

class db_to_voltage : public MpBase
{
public:
	db_to_voltage( IMpUnknown* host );
	void subProcess( int bufferOffset, int sampleFrames );
	virtual void onSetPins(void);

private:
	AudioInPin pindBin;
	AudioInPin pinVref;
	AudioOutPin pinVoltsout;
};

#endif

