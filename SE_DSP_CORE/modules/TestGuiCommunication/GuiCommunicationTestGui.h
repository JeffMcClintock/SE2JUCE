#ifndef GUICOMMUNICATIONTESTGUI_H_INCLUDED
#define GUICOMMUNICATIONTESTGUI_H_INCLUDED

#include "MP_SDK_GUI.h"

class GuiCommunicationTestGui : public SeGuiWindowsGfxBase
{
public:
	GuiCommunicationTestGui(IMpUnknown* host);

	// overrides
	virtual int32_t MP_STDCALL paint(HDC hDC);
	int32_t MP_STDCALL closeWindow( void );
	int32_t MP_STDCALL openWindow( HWND parentWindow, HWND& returnWindowHandle );
	int32_t onTimer();
	virtual int32_t MP_STDCALL receiveMessageFromAudio(int32_t id, int32_t size, void* messageData);

private:
 	void onUpdateFromDsp();
 	IntGuiPin fromDSP;

	int updateCount_;
	float updateHz_;
	int paintCount_;
	float repaintHz_;

	UINT_PTR timerId_;
	int repaintColour;
};

#endif


