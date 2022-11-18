#ifndef TEXTENTRY_H_INCLUDED
#define TEXTENTRY_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class TextEntry : public MpBase2
{
public:
	TextEntry( );
	virtual void onSetPins();

private:
	StringOutPin pinTextOut;
	StringInPin pinpatchValue;
};

#endif

