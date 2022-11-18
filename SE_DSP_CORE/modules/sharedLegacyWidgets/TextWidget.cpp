#include "TextWidget.h"
#include <algorithm>
#include "../se_sdk3/MpString.h"

using namespace gmpi;
using namespace gmpi_gui;
using namespace gmpi_sdk;
using namespace GmpiDrawing;

void TextWidget::OnRender(Graphics& g)
{
	Rect r = position;

#if 0 //def _DEBUG
	backgroundColor.a = 0.25f;
#endif

	// Background Fill.
	auto brush = g.CreateSolidColorBrush(backgroundColor);

	g.FillRectangle(r, brush);

	brush.SetColor(typeface_->getColor());

	static const float border = 1;
	Rect textRect(r);
	textRect.left += border; // backwards?
	textRect.right -= border;
	if (typeface_->verticalSnapBackwardCompatibilityMode)
	{
		textRect.top -= 2.0f;
	}

	// Center in cell.
	textRect.Offset(0.0f, textY);

	g.DrawTextU(text, dtextFormat, textRect, brush, (int32_t)DrawTextOptions::Clip);

//	_RPT4(_CRT_WARN, "DrawSize (%f,%f)\n", textRect.getWidth(), textRect.getHeight());
}

void TextWidget::Init(const char* style, bool centered)
{
	InitTextFormat(style, false, centered);

	backgroundColor = typeface_->getBackgroundColor();

	if (typeface_->verticalSnapBackwardCompatibilityMode)
	{
		textY = typeface_->getLegacyVerticalOffset(); // emulate GDI drawing
	}
	else
	{
		if(centered)
		{
			VerticalCenterText();
		}
	}
}

GmpiDrawing::Size TextWidget::getSize()
{
	auto widgetSize = dtextFormat.GetTextExtentU(text);

//	_RPT4(_CRT_WARN, "text extent'%s'", text.c_str());
//	_RPT2(_CRT_WARN, " text_size (%f,%f) \n", text_size.width, text_size.height);

	widgetSize.height = (float)typeface_->pixelHeight_;

	// allow for margins.
//? 	text_size.height += 2;
	widgetSize.width += 2; // from CMyEdit3::GetMinSize()

	return widgetSize;
}

