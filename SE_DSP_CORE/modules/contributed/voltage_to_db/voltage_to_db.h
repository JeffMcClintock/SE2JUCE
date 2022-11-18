#ifndef voltage_to_db_H_INCLUDED
#define voltage_to_db_H_INCLUDED

#include "mp_sdk_audio.h"

class voltage_to_db : public MpBase
{
public:
	voltage_to_db( IMpUnknown* host );
	void subProcess( int bufferOffset, int sampleFrames );
	virtual void onSetPins(void);

private:
	AudioInPin pinVoltsin;
	AudioInPin pinVref;
	AudioOutPin pindBout;
};

#endif

