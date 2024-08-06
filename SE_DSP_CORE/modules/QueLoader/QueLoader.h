// Copyright 2006 Jeff McClintock

#ifndef scope3_dsp_H_INCLUDED
#define scope3_dsp_H_INCLUDED

#include "mp_sdk_audio.h"

class QueLoader: public MpBase
{
	public:
	QueLoader(IMpUnknown* host);

	// overrides
	virtual int32_t MP_STDCALL open();
	virtual int32_t MP_STDCALL receiveMessageFromGui( int32_t id, int32_t size, void* messageData );

	// methods
	void subProcess( int bufferOffset, int sampleFrames );
	void onSetPins(void);

private:
	void sendResultToGui(int block_offset);
	bool messageDone;
	int count;

	// pins
	BlobInPin pinSamples;
	BlobInPin pinFromGui;
	FloatInPin pinRate;
};

#endif