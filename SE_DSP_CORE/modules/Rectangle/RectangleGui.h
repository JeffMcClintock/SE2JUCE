#ifndef RECTANGLEGUI_H_INCLUDED
#define RECTANGLEGUI_H_INCLUDED

#include "MP_SDK_GUI.h"

class RectangleGui : public SeGuiCompositedGfxBase
{
public:
	RectangleGui( IMpUnknown* host );

	// overrides
	virtual int32_t MP_STDCALL paint( HDC hDC );

private:
	void onRedraw();
	FloatGuiPin cornerRadius;
	BoolGuiPin TopLeft;
	BoolGuiPin TopRight;
	BoolGuiPin BottomLeft;
	BoolGuiPin BottomRight;
	StringGuiPin topColor;
	StringGuiPin bottomColor;
	StringGuiPin hint;
	StringGuiPin menuItems;
	IntGuiPin menuSelection;
	BoolGuiPin mouseDown;
};

#endif
