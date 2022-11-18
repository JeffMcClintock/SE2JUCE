#pragma once

#include <algorithm>
#include "../se_sdk3/mp_sdk_gui2.h"
#include "../shared/ImageMetadata.h"
#include "../sharedLegacyWidgets/BitmapWidget.h"
#include "../sharedLegacyWidgets/EditWidget.h"

class Slider2Gui : public gmpi_gui::MpGuiGfxBase
{
	const int editBoxWidth = 60;

	BitmapWidget bitmap;
	EditWidget edit;
	Widget* captureWidget;

public:
	Slider2Gui();

	virtual int32_t MP_STDCALL setHost(gmpi::IMpUnknown* host);
	virtual int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize);
	virtual int32_t MP_STDCALL arrange(GmpiDrawing_API::MP1_RECT finalRect) override;
	virtual int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext);

	virtual int32_t MP_STDCALL onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point);
	virtual int32_t MP_STDCALL onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point);
	virtual int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point);

	int32_t MP_STDCALL getToolTip(GmpiDrawing_API::MP1_POINT point, gmpi::IString* returnString) override
	{
		returnString->setData("Moose Face", 10);
		return gmpi::MP_OK;
	}

private:
//	void SetAnimationPosition(float p);
	void onSetValueIn();
	void onSetAppearance();
	void UpdateValuePinFromBitmap();
	void UpdateValuePinFromEdit();
	void UpdateEditText();

	FloatGuiPin pinValueIn;
	StringGuiPin pinItemList;

 	StringGuiPin pinNameIn;
 	StringGuiPin pinMenuItems;
 	IntGuiPin pinMenuSelection;
 	BoolGuiPin pinMouseDown;
 	FloatGuiPin pinRangeLo;
	FloatGuiPin pinRangeHi;
	FloatGuiPin pinResetValue;
	BoolGuiPin pinShowReadout;
	IntGuiPin pinAppearance;
};


