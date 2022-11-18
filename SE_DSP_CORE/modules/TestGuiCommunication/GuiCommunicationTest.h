#ifndef GUICOMMUNICATIONTEST_H_INCLUDED
#define GUICOMMUNICATIONTEST_H_INCLUDED

#include "mp_sdk_audio.h"

class GuiCommunicationTest : public MpBase
{
public:
	GuiCommunicationTest( IMpUnknown* host );
	void subProcess( int bufferOffset, int sampleFrames );
	virtual void onSetPins(void);
	virtual int32_t MP_STDCALL receiveMessageFromGui(int32_t id, int32_t size, const void* messageData);

private:
	IntOutPin toGui;
	FloatInPin updateRateHz;

	int counter_;
	int interval_;
};

#endif

