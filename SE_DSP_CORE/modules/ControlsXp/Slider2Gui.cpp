#include "./Slider2Gui.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include "../shared/xplatform_modifier_keys.h"
#include "../shared/unicode_conversion.h"
#include "./SliderGui.h"

using namespace std;
using namespace gmpi;
using namespace GmpiDrawing;
using namespace gmpi_gui;
using namespace JmUnicodeConversions;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, Slider2Gui, L"SE Slider2");

int32_t Slider2Gui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext)
{
	GmpiDrawing::Graphics dc(drawingContext);

//TODO	if (pinShowHeader)
//		header.OnRender(dc);

	bitmap.OnRender(dc);

	if( pinShowReadout )
		edit.OnRender(dc);

	return gmpi::MP_OK;
}

Slider2Gui::Slider2Gui() :
captureWidget ( nullptr)
{
	// initialise pins.
	initializePin( pinValueIn, static_cast<MpGuiBaseMemberPtr2>(&Slider2Gui::onSetValueIn));
	initializePin( pinNameIn);
	initializePin( pinMenuItems);
	initializePin( pinMenuSelection);
	initializePin( pinMouseDown);
	initializePin( pinRangeLo, static_cast<MpGuiBaseMemberPtr2>(&Slider2Gui::onSetValueIn));
	initializePin( pinRangeHi, static_cast<MpGuiBaseMemberPtr2>(&Slider2Gui::onSetValueIn));
	initializePin( pinResetValue);
	initializePin( pinAppearance, static_cast<MpGuiBaseMemberPtr2>(&Slider2Gui::onSetAppearance));
	initializePin(pinShowReadout);
}

void Slider2Gui::onSetAppearance()
{
	const char* imageFile = nullptr;
	auto toggleMode = skinBitmap::ToggleMode::Momentary;
	bool pixelAccurateHitTest = true;

	switch (pinAppearance)
	{
	case 0:
		imageFile = "vslider_med"; // Meant to be native slider.
		pixelAccurateHitTest = false;
		break;
	case 1:
		imageFile = "vslider_med";
		pixelAccurateHitTest = false;
		break;
	case 2:
		imageFile = "hslider_med";
		pixelAccurateHitTest = false;
		break;
	case 3:
		imageFile = "knob_med";
		break;
	case 4:
		imageFile = "button";
		break;
	case 5:
		imageFile = "button"; // toogle
		toggleMode = skinBitmap::ToggleMode::Alternate;
		break;
	case 6:
		imageFile = "button"; // native button.
		break;
	case 7:
		imageFile = "knob_sm";
		break;
	case 8:
		imageFile = "button_sm";
		break;
	case 9:
		imageFile = "button_sm"; // toogle
		toggleMode = skinBitmap::ToggleMode::Alternate;
		break;
	}

	bitmap.setHost(getHost());
	bitmap.Load( imageFile);
	bitmap.toggleMode2 = toggleMode;
	bitmap.setHitTestPixelAccurate(pixelAccurateHitTest);

	getGuiHost()->invalidateMeasure();
}

void Slider2Gui::onSetValueIn()
{
	if( !getCapture() )
	{
		bitmap.SetNormalised(( pinValueIn - pinRangeLo ) / ( pinRangeHi - pinRangeLo ));

		UpdateEditText();

		if( bitmap.ClearDirty() || edit.ClearDirty() )
			invalidateRect();
	}
	else // mouse currently held.
	{
// ?		SetAnimationPosition(animationPosition);
	}
}

void Slider2Gui::UpdateEditText()
{
	edit.SetText(WStringToUtf8(SliderGui::SliderFloatToString(pinValueIn)));

	if( edit.ClearDirty() )
		invalidateRect();
}

int32_t Slider2Gui::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
//	_RPT2(_CRT_WARN, "onPointerDown (%f,%f)\n", point.x, point.y);

	// Alt-left-click to reset.
	if ((flags & gmpi_gui_api::GG_POINTER_KEY_ALT) != 0 && (flags & gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON) != 0)
	{
		pinValueIn = pinResetValue;

		onSetValueIn();

		return gmpi::MP_OK;
	}

	captureWidget = nullptr;

	int hitWidget = -1;

	if (bitmap.bitmapHitTest2(point))
	{
		hitWidget = 0;
	}
	if (edit.widgetHitTest(point))
	{
		hitWidget = 1;
	}
	/*
	if (headerWidget.widgetHitTest(point))
	{
		hitWidget = 2;
		return gmpi::MP_OK; // OK to select/drag.
	}
	*/

	if (hitWidget < 0)
	{
		return gmpi::MP_UNHANDLED; // hit test failed.
	}

	// Let host handle right-clicks.
	if ((flags & gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON) == 0)
	{
		return gmpi::MP_OK; // right-click hit-test succeeded.
	}

	// Alt-click to reset.
//	if (modifier_keys::isHeldAlt())
	if (flags & gmpi_gui_api::GG_POINTER_KEY_ALT)
	{
		pinValueIn = pinResetValue;

		onSetValueIn();

		return gmpi::MP_OK;
	}

	if (hitWidget == 0)
	{
		if (bitmap.onPointerDown(flags, point))
		{
			captureWidget = &bitmap;
			setCapture();
		}

		// Update in case of clicks.
		UpdateValuePinFromBitmap();

		if (bitmap.ClearDirty())
			invalidateRect();
	}

	if (hitWidget == 1)
	{
		if (edit.onPointerDown(flags, point))
		{
			captureWidget = &edit;
			setCapture();
		}
	}

	return hitWidget >= 0 ? gmpi::MP_HANDLED : gmpi::MP_UNHANDLED;
}

int32_t Slider2Gui::onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
//	_RPT2(_CRT_WARN, "onPointerMove (%f,%f)\n", point.x, point.y);

	if (!getCapture())
	{
		return gmpi::MP_UNHANDLED;
	}

	if( captureWidget )
	{
		captureWidget->onPointerMove(flags, point);

		UpdateValuePinFromBitmap();

		if( bitmap.ClearDirty() )
			invalidateRect();
	}

	return gmpi::MP_OK;
}

int32_t Slider2Gui::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
//	_RPT2(_CRT_WARN, "onPointerUp (%f,%f)\n", point.x, point.y);

	if (!getCapture())
	{
		return gmpi::MP_UNHANDLED;
	}

	releaseCapture();

	if( captureWidget )
	{
		captureWidget->onPointerUp(flags, point);

		if( captureWidget == &bitmap )
		{
			UpdateValuePinFromBitmap();
		}
		else
		{
			UpdateValuePinFromEdit();
		}

		if( bitmap.ClearDirty() || edit.ClearDirty() )
			invalidateRect();
	}

	return gmpi::MP_OK;
}

void Slider2Gui::UpdateValuePinFromBitmap()
{
	pinValueIn = bitmap.GetNormalised() * ( pinRangeHi - pinRangeLo ) + pinRangeLo;
	UpdateEditText();
}

void Slider2Gui::UpdateValuePinFromEdit()
{
	try {
		pinValueIn = (float)stod(edit.GetText());
	}
	catch( const std::invalid_argument& ) {
//		std::cerr << "Invalid argument: " << ia.what() << '\n';
	}

	onSetValueIn();
}

int32_t Slider2Gui::measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize)
{
	edit.setHost(getHost());
	edit.Init("control_edit");

	auto bmSize = bitmap.getSize();
	auto edSize = edit.getSize();

	returnDesiredSize->width = ( std::max )( bmSize.width, (float) editBoxWidth );
//	returnDesiredSize->width = ( std::max )( returnDesiredSize->width, availableSize.x ); // stretch to fit H.
	returnDesiredSize->height = bmSize.height;
	
	if( pinShowReadout )
		returnDesiredSize->height += edSize.height;

	return gmpi::MP_OK;

	const int prefferedSize = 100;
	const int minSize = 15;

	returnDesiredSize->width = availableSize.width;
	returnDesiredSize->height = availableSize.height;
	if (returnDesiredSize->width > prefferedSize)
	{
		returnDesiredSize->width = prefferedSize;
	}
	else
	{
		if (returnDesiredSize->width < minSize)
		{
			returnDesiredSize->width = minSize;
		}
	}
	if (returnDesiredSize->height > prefferedSize)
	{
		returnDesiredSize->height = prefferedSize;
	}
	else
	{
		if (returnDesiredSize->height < minSize)
		{
			returnDesiredSize->height = minSize;
		}
	}
	return gmpi::MP_OK;
}

int32_t Slider2Gui::arrange(GmpiDrawing_API::MP1_RECT finalRect_s)
{
	Rect finalRect(finalRect_s);
	float y = 0;
	float x = 0;

	Size s(0,0);
	if (pinAppearance >= 0)
	{
		s = bitmap.getSize();
		x = floorf((finalRect.getWidth() - s.width) / 2);
		// Bitmap at top. Centered.
		bitmap.setPosition(Rect(x, y, x + s.width, y + s.height));
		y += s.height;
	}

	float remainingVertical = finalRect.getHeight() - y;

	// Edit below. Centered.
	s.width = (float)editBoxWidth;
	s.height = (std::min)( edit.getSize().height, remainingVertical);
	x = floorf(0.5f + (finalRect.getWidth() - s.width) * 0.5f); // center.

	edit.setPosition(Rect(x, y, x + s.width, y + s.height));

	return MpGuiGfxBase::arrange(finalRect);
}

int32_t Slider2Gui::setHost(gmpi::IMpUnknown* host)
{
	bitmap.setHost(host);
	edit.setHost(host);
	return MpGuiGfxBase::setHost(host);
}
