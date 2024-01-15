#ifndef RECTANGLEGUI_H_INCLUDED
#define RECTANGLEGUI_H_INCLUDED

#include "../se_sdk3/Drawing.h"
#include "mp_sdk_gui2.h"

class RectangleGui : public gmpi_gui::MpGuiGfxBase
{
public:
	RectangleGui();

	void onChangeRadius();
	void onChangeTopCol();
	void onChangeBotCol();

	// overrides.
	int32_t MP_STDCALL arrange(GmpiDrawing_API::MP1_RECT finalRect) override;
	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override;

private:
	void onRedraw();

	FloatGuiPin pinCornerRadius;
 	BoolGuiPin pinTopLeft;
 	BoolGuiPin pinTopRight;
 	BoolGuiPin pinBottomLeft;
 	BoolGuiPin pinBottomRight;
 	StringGuiPin pinTopColor;
 	StringGuiPin pinBottomColor;

	GmpiDrawing::Color topColor;
	GmpiDrawing::Color bottomColor;

	GmpiDrawing::PathGeometry geometry;
};

#endif


