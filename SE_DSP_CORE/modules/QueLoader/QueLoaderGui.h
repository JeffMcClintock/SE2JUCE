#pragma once

#include "mp_sdk_gui.h"

class QueLoader_gui :
	public SeGuiWindowsGfxBase
{
public:
	QueLoader_gui(IMpUnknown* host);

	// overrides
	virtual int32_t MP_STDCALL paint(HDC hDC);
	virtual int32_t MP_STDCALL measure(MpSize availableSize, MpSize& returnDesiredSize);
	virtual int32_t MP_STDCALL receiveMessageFromAudio( int32_t id, int32_t size, void* messageData );

	virtual int32_t MP_STDCALL openWindow( HWND parentWindow, HWND& returnWindowHandle );
	virtual int32_t MP_STDCALL closeWindow( void );

	void onValueChanged(int index);
	int32_t onTimer();
	BlobArrayGuiPin pinSamples;
	BlobGuiPin pinToDsp;
	FloatGuiPin pinRate;

private:
	UINT_PTR timerId_;
	bool messageDone;
};
