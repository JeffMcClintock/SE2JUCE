#include "./TextEntry4Gui.h"
#include "../shared/unicode_conversion.h"
#include "../se_sdk3/it_enum_list.h"
#include "../se_sdk3/MpString.h"
#include <algorithm>

using namespace gmpi;
using namespace gmpi_gui;
using namespace gmpi_sdk;
using namespace JmUnicodeConversions;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;
using namespace gmpi_gui_api;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, TextEntry4Gui, L"SE Text Entry4" );
SE_DECLARE_INIT_STATIC_FILE(TextEntry4_Gui);

TextEntry4Gui::TextEntry4Gui()
{
	// initialise pins.
	initializePin( pinText, static_cast<MpGuiBaseMemberPtr2>(&TextEntry4Gui::redraw) );
	initializePin( pinStyle, static_cast<MpGuiBaseMemberPtr2>(&TextEntry4Gui::onSetStyle) );
	initializePin( pinMultiline, static_cast<MpGuiBaseMemberPtr2>(&TextEntry4Gui::onSetStyle) );
	initializePin( pinWriteable );
	initializePin( pinGreyed, static_cast<MpGuiBaseMemberPtr2>(&TextEntry4Gui::redraw) );
	initializePin( pinHint );
	initializePin( pinMenuItems );
	initializePin( pinMenuSelection );
	initializePin( pinMouseDown_LEGACY );
	initializePin( pinMouseDown );
}

void TextEntry4Gui::onSetStyle()
{
	std::string customStyleName{"TextEntry4Gui:text:"};
	customStyleName += WStringToUtf8(pinStyle.getValue());

	if(pinMultiline.getValue())
	{
		customStyleName += ":multiline";
	}

	fontmetadata = nullptr;
	textFormat = FontCache::instance()->TextFormatExists(getHost(), getGuiHost(), customStyleName, &fontmetadata);
	if (!textFormat)
	{
		// Get the specified font style.
		GetTextFormat(getHost(), getGuiHost(), pinStyle, &fontmetadata);

		auto textFormatMutable = AddCustomTextFormat(getHost(), getGuiHost(), customStyleName, fontmetadata);

		// Apply customisation.
		if(pinMultiline.getValue())
		{
			textFormatMutable.SetWordWrapping(WordWrapping::Wrap);
		}
		else
		{
			textFormatMutable.SetWordWrapping(WordWrapping::NoWrap);
		}
		textFormat = textFormatMutable.Get();
	}

	TextSubcontrol::onSetStyle();
}

// Overriden to suport multiline text.
int32_t TextEntry4Gui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext)
{
	GmpiDrawing::Graphics g(drawingContext);

	auto r = getRect();
	Rect textRect(r);
	textRect.Deflate((float)border, (float)border);

	// Background Fill. Currently fills behind frame line too (could be more efficient).
	auto brush = g.CreateSolidColorBrush(fontmetadata->getBackgroundColor());
	g.FillRectangle(textRect, brush);

	if (pinGreyed == true)
		brush.SetColor(Color::Gray);
	else
		brush.SetColor(fontmetadata->getColor());

	if(fontmetadata->verticalSnapBackwardCompatibilityMode)
	{
		auto directXOffset = fontmetadata->getLegacyVerticalOffset();
		textRect.top += directXOffset;
		textRect.bottom += directXOffset;
	}

	g.DrawTextU(getDisplayText(), textFormat, textRect, brush, (int32_t)DrawTextOptions::Clip);

	return gmpi::MP_OK;
}

int32_t TextEntry4Gui::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	// Let host handle right-clicks.
	if( ( flags & GG_POINTER_FLAG_FIRSTBUTTON ) == 0 )
	{
		return gmpi::MP_OK; // Indicate successful hit, so right-click menu can show.
	}

	setCapture();

	pinMouseDown = true;
	pinMouseDown_LEGACY = true;

	return gmpi::MP_OK;
}

int32_t TextEntry4Gui::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	if( !getCapture() )
	{
		return gmpi::MP_UNHANDLED;
	}

	releaseCapture();

	pinMouseDown = false;
	pinMouseDown_LEGACY = false;

	if (pinWriteable == false)
		return gmpi::MP_OK;

	GmpiGui::GraphicsHost host(getGuiHost());
	nativeEdit = host.createPlatformTextEdit(getRect());
	nativeEdit.SetAlignment(fontmetadata->getTextAlignment(), pinMultiline.getValue() ? WordWrapping::Wrap : WordWrapping::NoWrap);
	nativeEdit.SetTextSize((float)fontmetadata->size_);// GetFontMetatdata(pinStyle)->size_);
	nativeEdit.SetText(WStringToUtf8(pinText).c_str());
	nativeEdit.ShowAsync([this](int32_t result) -> void { this->OnTextEnteredComplete(result); });

	return gmpi::MP_OK;
}

std::string TextEntry4Gui::getDisplayText()
{
	return WStringToUtf8(pinText.getValue());
}

void TextEntry4Gui::OnTextEnteredComplete(int32_t result)
{
	if (result == gmpi::MP_OK)
	{
		pinText = Utf8ToWstring( nativeEdit.GetText() );
		invalidateRect();
	}

	nativeEdit.setNull(); // release it.
}
