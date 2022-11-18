#ifndef GUICOMTEST2_H_INCLUDED
#define GUICOMTEST2_H_INCLUDED

#include "mp_sdk_audio.h"

class GuiComTest2 : public MpBase
{
	int clickCount = 0;
public:
	GuiComTest2( IMpUnknown* host );
	void onSetPins(void) override;
	int32_t MP_STDCALL receiveMessageFromGui( int32_t id, int32_t size, const void* messageData ) override;

private:
	AudioInPin pinIn;
	AudioOutPin pinOut;
};

#endif

