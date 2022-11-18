#ifndef GUICOMTEST2GUI_H_INCLUDED
#define GUICOMTEST2GUI_H_INCLUDED

#include "MP_SDK_GUI.h"

class GuiComTest2Gui : public SeGuiWindowsGfxBase
{
public:
	GuiComTest2Gui(IMpUnknown* host);
	virtual int32_t MP_STDCALL onLButtonDown( UINT flags, POINT point );

private:
 	void onSetCaptureDataA();
 	void onSetCaptureDataB();
 	void onSetVoiceGate();
 	void onSetpolydetect();
 	BlobGuiPin captureDataA;
 	BlobGuiPin captureDataB;
 	FloatGuiPin voiceGate;
 	BoolGuiPin polydetect;
};

#endif


