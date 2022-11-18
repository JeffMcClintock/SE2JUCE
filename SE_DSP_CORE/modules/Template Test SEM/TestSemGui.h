#ifndef TESTSEMGUI_H_INCLUDED
#define TESTSEMGUI_H_INCLUDED

#include "Drawing.h"
#include "mp_sdk_gui2.h"

class TestSemGui : public gmpi_gui::MpGuiGfxBase
{
public:
	TestSemGui();

	// overrides.
	virtual int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override;

private:
 	void onSetCaptureDataA();
 	void onSetCaptureDataB();
 	void onSetVoiceGate();
 	void onSetpolydetect();
 	BlobGuiPin pinCaptureDataA;
 	BlobGuiPin pinCaptureDataB;
 	FloatGuiPin pinVoiceGate;
 	BoolGuiPin pinpolydetect;
};

#endif


