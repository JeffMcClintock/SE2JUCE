#include "./SliderGui.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include "../shared/xplatform_modifier_keys.h"
#include "../shared/unicode_conversion.h"
#include "../shared/it_enum_list.h"

using namespace std;
using namespace gmpi;
using namespace gmpi_gui;
using namespace GmpiDrawing;
using namespace JmUnicodeConversions;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, SliderGui, L"SE Slider");
SE_DECLARE_INIT_STATIC_FILE(Slider_Gui);

int32_t SliderGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext)
{
	GmpiDrawing::Graphics dc(drawingContext);

	headerWidget.OnRender(dc);
	bitmap.OnRender(dc);

	if( pinShowReadout.getValue() )
		edit.OnRender(dc);

	return gmpi::MP_OK;
}

SliderGui::SliderGui() :
captureWidget ( nullptr)
{
	// initialise pins.
	initializePin( 13, pinValueIn, static_cast<MpGuiBaseMemberPtr2>(&SliderGui::onSetValueIn));
	initializePin(pinHint);
	initializePin(pinAppearance, static_cast<MpGuiBaseMemberPtr2>(&SliderGui::onSetAppearance));
	initializePin(pinTitle, static_cast<MpGuiBaseMemberPtr2>(&SliderGui::onSetTitle));// , static_cast<MpGuiBaseMemberPtr2>(&SliderGui::onSetTitle));
	initializePin(pinNormalised);

//	initializePin(pinNameIn);
	initializePin(pinMenuItems);
	initializePin( pinMenuSelection);
	initializePin( pinMouseDown);
	initializePin( pinRangeLo, static_cast<MpGuiBaseMemberPtr2>(&SliderGui::onSetValueIn));
	initializePin( pinRangeHi, static_cast<MpGuiBaseMemberPtr2>(&SliderGui::onSetValueIn));
//	initializePin( pinResetValue);
	initializePin(pinShowReadout);
//	initializePin(pinShowTitle);
}

void SliderGui::onSetAppearance()
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
		imageFile = "button"; // toggle
		toggleMode = skinBitmap::ToggleMode::Alternate;
		break;
	case 7:
		imageFile = "knob_sm";
		break;
	case 6: // native button.
	case 8:
		imageFile = "button_sm";
		break;
	case 9:
		imageFile = "button_sm"; // toggle
		toggleMode = skinBitmap::ToggleMode::Alternate;
		break;
	}

	// Discard any previous image cache.
	getHost()->ClearResourceUris();

	bitmap.setHost(getHost());
	bitmap.Load( imageFile);
	//bitmap.SetToggle(toggleMode);
	bitmap.toggleMode2 = toggleMode;

	bitmap.setHitTestPixelAccurate(pixelAccurateHitTest);

	getGuiHost()->invalidateMeasure();
}

std::wstring SliderGui::SliderFloatToString(float val, int p_decimal_places) // better because it removes trailing zeros. -1 for auto decimal places
{
	bool auto_decimal = p_decimal_places < 0; // remember auto (as decimal places is modified)
	std::wostringstream oss;
	oss << setiosflags(ios_base::fixed);

	if( auto_decimal )
	{
		// attempt to see enough of number when smallish.
		int precision = 3;
		float absValue = fabsf(val);
		if (absValue > 0.00000001f)
		{
			absValue *= 100.0f;

			while (precision < 8 && absValue < 1.0f )
			{
				++precision;
				absValue *= 10.0f;
			}
		}

		oss << setprecision(precision);
	}
	else
	{
		oss << setprecision(p_decimal_places); // x significant digits AFTER point.
	}

	oss << val;

	//std::wstring s;
	//s.Format( (L"%.*f"), p_decimal_places, (double)val );
	std::wstring s = oss.str();

	// Replace -0.0 with 0.0 ( same for -0.00 and -0.000 etc).
	// deliberate 'feature' of printf is to round small negative numbers to -0.0
	if( s[0] == L'-' && val > -1.0f )
	{
		int i = (int)s.size() - 1; // Not using unsigned size_t else fails <0 test below.

		while( i > 0 )
		{
			if( s[i] != L'0' && s[i] != L'.' )
			{
				break;
			}

			--i;
		}

		if( i == 0 ) // nothing but zeros (or dot).
		{
			s = s.substr(1);
		}
	}

	return s;
}

void SliderGui::onSetValueIn()
{
	if( !getCapture() )
	{
		bitmap.SetNormalised(( pinValueIn - pinRangeLo ) / ( pinRangeHi - pinRangeLo ));

		UpdateEditText();

		if( bitmap.ClearDirty() || edit.ClearDirty() )//|| header.ClearDirty() )
			invalidateRect();
	}
	else // mouse currently held.
	{
// ?		SetAnimationPosition(animationPosition);
	}
}

void SliderGui::onSetTitle()
{
	headerWidget.SetText(pinTitle);

	if (headerWidget.ClearDirty())
		invalidateMeasure();
}

void SliderGui::UpdateEditText()
{
	edit.SetText(WStringToUtf8(SliderFloatToString(pinValueIn)));

	if( edit.ClearDirty() )
		invalidateRect();
}

int32_t SliderGui::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
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
	if (headerWidget.widgetHitTest(point))
	{
		hitWidget = 2;
		return gmpi::MP_OK; // OK to select/drag.
	}

	if (hitWidget < 0)
	{
		return gmpi::MP_UNHANDLED; // hit test failed.
	}

	// Let host handle right-clicks.
	if ((flags & gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON) == 0)
	{
		return gmpi::MP_OK; // right-click hit-test succeeded.
	}

	/*
	// Alt-click to reset.
//	if (modifier_keys::isHeldAlt())
	if (flags & gmpi_gui_api::GG_POINTER_KEY_ALT)
	{
		pinValueIn = pinResetValue;

		onSetValueIn();

		return gmpi::MP_OK;
	}
	*/
	if (hitWidget == 0)
	{
		if (bitmap.onPointerDown(flags, point))
		{
			captureWidget = &bitmap;
			pinMouseDown = true;
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

int32_t SliderGui::onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point)
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

int32_t SliderGui::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
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
			pinMouseDown = false;
		}
		else
		{
			// not quite rigth in asnc world. Need callback from widget.
			UpdateValuePinFromEdit();
		}

		if( bitmap.ClearDirty() || edit.ClearDirty() )
			invalidateRect();
	}

	return gmpi::MP_OK;
}

int32_t SliderGui::onMouseWheel(int32_t flags, int32_t delta, GmpiDrawing_API::MP1_POINT point)
{
	// ignore horizontal scrolling
	if (0 != (flags & gmpi_gui_api::GG_POINTER_KEY_SHIFT))
		return gmpi::MP_UNHANDLED;

	const float scale = (flags & gmpi_gui_api::GG_POINTER_KEY_CONTROL) ? 1.0f / 12000.0f : 1.0f / 1200.0f;
	const float newval = bitmap.GetNormalised() + delta * scale;
	bitmap.SetNormalised(std::clamp(newval, 0.0f, 1.0f));
	UpdateValuePinFromBitmap();

	return gmpi::MP_OK;
}

int32_t SliderGui::populateContextMenu(float x, float y, gmpi::IMpUnknown* contextMenuItemsSink)
{
	// TODO: hit-test.

	gmpi::IMpContextItemSink* sink;
	contextMenuItemsSink->queryInterface(gmpi::MP_IID_CONTEXT_ITEMS_SINK, reinterpret_cast<void**>(&sink));

	it_enum_list itr(pinMenuItems);

	for (itr.First(); !itr.IsDone(); ++itr)
	{
		int32_t flags = 0;

		// Special commands (sub-menus)
		switch (itr.CurrentItem()->getType())
		{
			case enum_entry_type::Separator:
			case enum_entry_type::SubMenu:
				flags |= gmpi_gui::MP_PLATFORM_MENU_SEPARATOR;
				break;

			case enum_entry_type::SubMenuEnd:
			case enum_entry_type::Break:
				continue;

			default:
				break;
		}

		sink->AddItem(WStringToUtf8(itr.CurrentItem()->text).c_str(), itr.CurrentItem()->value, flags);
	}

	return gmpi::MP_OK;
}

int32_t SliderGui::onContextMenu(int32_t selection)
{
	pinMenuSelection = selection; // send menu momentarily, then reset.
	pinMenuSelection = -1;

	return gmpi::MP_OK;
}

void SliderGui::UpdateValuePinFromBitmap()
{
	pinValueIn = bitmap.GetNormalised() * ( pinRangeHi - pinRangeLo ) + pinRangeLo;
	UpdateEditText();
}

void SliderGui::UpdateValuePinFromEdit()
{
	try {
		pinValueIn = (float)stod(edit.GetText());
	}
	catch( const std::invalid_argument& ) {
//		std::cerr << "Invalid argument: " << ia.what() << '\n';
	}

	onSetValueIn();
}

int32_t SliderGui::measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize)
{
	auto bmSize = bitmap.getSize();
	auto hdSize = headerWidget.getSize();

	returnDesiredSize->height = bmSize.height + hdSize.height;
	returnDesiredSize->width = (std::max)(bmSize.width, hdSize.width);

	if (pinShowReadout.getValue())
	{
		auto edSize = edit.getSize();
//		edSize.width = editBoxWidth; // this setting override default width.
		returnDesiredSize->height += edSize.height;
		returnDesiredSize->width = (std::max)(returnDesiredSize->width, edSize.width);
	}

#ifdef _DEBUG
	debug_IsMeasured = true;
#endif

	return gmpi::MP_OK;
}

int32_t SliderGui::arrange(GmpiDrawing_API::MP1_RECT finalRect_s)
{
	Rect finalRect(finalRect_s);

	float y = 0;
	float x = 0;

	Size s(0, 0);

	// Header.
	{
		s = headerWidget.getSize();
		x = floorf((finalRect.getWidth() - s.width) / 2);
		// At top. Centered.
		headerWidget.setPosition(Rect(x, y, x + s.width, y + s.height));
		y += s.height;
	}

	if (pinAppearance >= 0)
	{
		s = bitmap.getSize();
		// was done with integer arithmetic in GDI.
		x = floorf((finalRect.getWidth() - s.width) / 2);
		// Bitmap at top. Centered.
		bitmap.setPosition(Rect(x, y, x + s.width, y + s.height));
		y += s.height;
	}

	float remainingVertical = finalRect.getHeight() - y;

	// Edit below. Centered.
	s.width = edit.getSize().width; // (float)editBoxWidth;
	s.height = (std::min)(edit.getSize().height, remainingVertical);
	x = floorf(0.5f + (finalRect.getWidth() - s.width) * 0.5f); // center.

	edit.setPosition(Rect(x, y, x + s.width, y + s.height));

	return MpGuiGfxBase::arrange(finalRect);
}

int32_t SliderGui::setHost(gmpi::IMpUnknown* host)
{
	bitmap.setHost(host);
	edit.setHost(host);
	headerWidget.setHost(host);

	const auto r =  MpGuiGfxBase::setHost(host);

	// In SE 1.1 List-Entry had centered headings.
	// In 1.3 everything got left-justified by mistake.Rectify this, but only if (1.4) backward-compatibility switched off.
	{
		FontMetadata* fontData{};
		FontCache::instance()->GetTextFormat(getHost(), getGuiHost(), "control_label", &fontData);

		backwardCompatibleVerticalArrange = fontData->verticalSnapBackwardCompatibilityMode;
	}

	return r;
}

int32_t SliderGui::initialize()
{
	edit.SetMinVisableChars(5); // from Ctl_Slider::BuildWidget2()
	edit.Init("control_edit", true);
	edit.OnChangedEvent = [this](const char* newvalue) -> void
		{
			try
			{
				pinValueIn = (float)stod(edit.GetText());
			}
			catch (const std::invalid_argument&) {
				//		std::cerr << "Invalid argument: " << ia.what() << '\n';
			}

			onSetValueIn();
		};

	const bool centerHeading  = !backwardCompatibleVerticalArrange;
	headerWidget.Init("control_label", centerHeading);
	headerWidget.SetText(pinTitle);

	return gmpi_gui::MpGuiGfxBase::initialize();
}
