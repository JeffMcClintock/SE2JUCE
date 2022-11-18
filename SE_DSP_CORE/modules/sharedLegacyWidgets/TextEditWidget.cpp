#include "TextEditWidget.h"
#include <algorithm>
#include "../se_sdk3/MpString.h"

using namespace gmpi;
using namespace gmpi_gui;
using namespace gmpi_sdk;
using namespace GmpiDrawing;

void TextEditWidget::OnRender(Graphics& dc)
{
	Rect r = position;

	// Background Fill.
	auto brush = dc.CreateSolidColorBrush(Color::FromBytes(238, 238, 238));
	dc.FillRectangle(r, brush);

	// Outline.
	Rect borderRect(r);
	borderRect.Deflate(0.5f);
	brush.SetColor(Color::FromBytes(150, 150, 150));
	dc.DrawRectangle(borderRect, brush);

	brush.SetColor(Color::Black);

	static const float border = 1;
	Rect textRect(r);
	textRect.left += border + 1;
	textRect.right -= border;

	if(typeface_->verticalSnapBackwardCompatibilityMode)
	{
		textRect.top -= 1; // emulate GDI drawing
	}
	else
	{
		textRect.bottom = textRect.top + textHeight;
	}

	textRect.Offset(0.0f, textY);

	dc.DrawTextU(text, dtextFormat, textRect, brush, (int32_t)DrawTextOptions::Clip);
}

void TextEditWidget::Init(const char* style)
{
	InitTextFormat(style, false, false);

	if (typeface_->verticalSnapBackwardCompatibilityMode)
	{
		textY = typeface_->getLegacyVerticalOffset(); // emulate GDI drawing
	}
	else
	{
		VerticalCenterText(false);
	}
}

GmpiDrawing::Size TextEditWidget::getSize()
{
//	return Size(8.0f, ceilf(typeface_->pixelHeight_) + 4); // + GetSystemMetrics(SM_CYEDGE) * 4 + hack.
	return Size(8.0f, static_cast<float>(typeface_->pixelHeight_));
}

bool TextEditWidget::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	// interested only in right-mouse clicks.
	return ((flags & (gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON | gmpi_gui_api::GG_POINTER_FLAG_NEW)) == (gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON | gmpi_gui_api::GG_POINTER_FLAG_NEW));
}

void TextEditWidget::OnPopupmenuComplete(int32_t result)
{
	if (result == gmpi::MP_OK)
	{
		OnChangedEvent(nativeWidget.GetText());
	}

	nativeWidget.setNull(); // release it.
}

void TextEditWidget::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	GmpiGui::GraphicsHost host(getGuiHost());
	nativeWidget = host.createPlatformTextEdit(position);
	nativeWidget.SetAlignment(GmpiDrawing::TextAlignment::Trailing);
	nativeWidget.SetTextSize(static_cast<float>(typeface_->size_));
	nativeWidget.SetText(text);

	nativeWidget.ShowAsync(&onPopupMenuCompleteEvent);
}