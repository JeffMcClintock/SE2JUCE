#ifndef GRAPHICSFRAMERATEGUI_H_INCLUDED
#define GRAPHICSFRAMERATEGUI_H_INCLUDED

#include <chrono>
#include "../se_sdk3/Drawing.h"
#include "mp_sdk_gui2.h"
#include "../se_sdk3/TimerManager.h"

class GraphicsFrameRateGui : public gmpi_gui::MpGuiGfxBase, public TimerClient
{
	std::chrono::steady_clock::time_point lastDrawTime;
	char text[20];

	GmpiDrawing::TextFormat textFormat;
	GmpiDrawing::SolidColorBrush brush;
	float framesPerSecond;

public:
	GraphicsFrameRateGui();

	// overrides.
	virtual int32_t MP_STDCALL initialize() override;
	virtual int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override;

	virtual bool OnTimer() override;
};

#endif


