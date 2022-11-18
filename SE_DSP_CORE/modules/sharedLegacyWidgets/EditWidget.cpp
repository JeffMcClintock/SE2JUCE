#include "EditWidget.h"
#include <algorithm>
#include "../shared/unicode_conversion.h"
#include "../se_sdk3/MpString.h"

using namespace std;
using namespace gmpi;
using namespace gmpi_gui;
using namespace gmpi_sdk;
using namespace JmUnicodeConversions;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;

Size EditWidget::default_text_size;

EditWidget::EditWidget() :
readOnly(false)
, onTextEntryCompeteEvent([this](int32_t result) -> void { this->OnTextEnteredComplete(result); })
, m_visable_chars(-1) // signifies to use default for datatype
{
}

void EditWidget::OnRender(Graphics& g)
{
	Rect r = position;

	ClipDrawingToBounds c(g, r);

	// Background Fill.
	auto brush = g.CreateSolidColorBrush(Color::White);
	g.FillRectangle(r, brush);

	// Outline.
	{
		const float penWidth = 1;
		auto factory = g.GetFactory();
		auto geometry = factory.CreatePathGeometry();
		auto sink = geometry.Open();

		float offset = 0.5f;
		r.top += offset;
		r.left += offset;
		r.bottom -= offset;
		r.right -= offset;

		Point p(r.left, r.top);

		sink.BeginFigure(p, FigureBegin::Filled);

		sink.AddLine(Point(r.right, r.top));
		sink.AddLine(Point(r.right, r.bottom));
		sink.AddLine(Point(r.left, r.bottom));

		sink.EndFigure();
		sink.Close();

		brush.SetColor(Color::FromBytes(100, 100, 100));

		g.DrawGeometry(geometry, brush, penWidth);
	}

	Rect textRect(r);
#if 0
{
	float y = (r.top + r.bottom) * 0.5f;
	g.DrawLine(0.0f, y, 100.0f, y, brush, 0.01f);

	GmpiDrawing_API::MP1_FONT_METRICS fontMetrics;
	dtextFormat.GetFontMetrics(&fontMetrics);
	float actualtop = r.top + textY;
	float digitTop = fontMetrics.ascent - fontMetrics.capHeight;
	y = actualtop + digitTop;
	g.DrawLine(10.0f, y, 100.0f, y, brush, 0.01f);
	y = actualtop + digitTop + fontMetrics.capHeight * 0.5f;
	g.DrawLine(10.0f, y, 100.0f, y, brush, 0.01f);
	y = actualtop + fontMetrics.ascent;
	g.DrawLine(10.0f, y, 100.0f, y, brush, 0.01f);

	textRect.bottom = textRect.top + fontMetrics.bodyHeight();
	auto s = dtextFormat.GetTextExtentU("8");
	int jjj = 9;
}
text = "Agj|8880.3";
#endif

	// Current selection text.
	brush.SetColor(Color::FromBytes(0, 0, 50));

	static const int border = 2;
	textRect.left += border;
	textRect.right -= border;

	if (!typeface_->verticalSnapBackwardCompatibilityMode)
	{
		textRect.bottom = textRect.top + textHeight;
	}

	// Center in cell.
	textRect.Offset(0.0f, textY);

	auto brush2 = g.CreateSolidColorBrush(Color::FromBytes(0, 0, 255));
	g.DrawTextU(text, dtextFormat, textRect, brush2);
}

void EditWidget::Init(const char* style, bool digitsOnly)
{
	// Cache default EditWidget size, based on default text format.
	if(default_text_size.height == 0.0f)
	{
		// NOTE: This is completely the wrong font. Effectivly it just gets the fallback font.
		// We already have a member 'typeface' with the correct font.
		// This is here only for calculating the widget size for backwards compatibility.
		FontMetadata* fontData{};
		GetTextFormat(getHost(), getGuiHost(), "Custom:EditWidget", &fontData);
		default_text_size = Size((float)fontData->pixelWidth_, (float)fontData->pixelHeight_);
	}

	const auto outlineHeight = default_text_size.height - 1.0f; // when drawing we shrink outline 1/2 pixel each side.
	const auto padding = 1.0f; // space between text and outline, doubled.
	const auto textCellHeight = outlineHeight - padding;

	InitTextFormat(style, digitsOnly, false, textCellHeight);

	if (!typeface_->verticalSnapBackwardCompatibilityMode)
	{
		VerticalCenterText(digitsOnly, 0.5f);
	}
}

GmpiDrawing::Size EditWidget::getSize()
{
	int vc = m_visable_chars;

	if( vc == -1 ) // use default
	{
		if( false )// m_datatype == DT_TEXT )
		{
			vc = 6;
		}
		else
		{
			vc = 4;
		}
	}

	Size text_size = default_text_size;

	if(!typeface_->verticalSnapBackwardCompatibilityMode)
	{
		// With new sizing, retain width from legacy version, to preserve peoples layout as much as practical.
		// Use corrected cell height though to ensure text centered vertically.
		text_size.height = (float)typeface_->pixelHeight_;
	}

	float minWidth = 4;
	if( vc > 0 )
	{
		text_size.width += 2;
		text_size.width *= vc;
// not in  CMyEdit::SeMeasure():		text_size.height += 2; // allow for margins and window border

		minWidth = (std::max)((float)typeface_->pixelWidth_, minWidth );
	}

	return text_size;
}

bool EditWidget::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	return true;
}

void EditWidget::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	if( readOnly )
		return;

	GmpiDrawing::Rect r = getRect();
	gmpi_sdk::mp_shared_ptr<gmpi_gui::IMpPlatformText> returnObject;
	getGuiHost()->createPlatformTextEdit(&r, returnObject.getAddressOf());

	if( returnObject == 0 )
	{
		return;
	}

	returnObject->queryInterface(SE_IID_GRAPHICS_PLATFORM_TEXT, nativeEdit.asIMpUnknownPtr());

	if( nativeEdit == 0 )
	{
		return;
	}

	// TODO: !!! set font (at least height).
	nativeEdit->SetAlignment((int32_t) GmpiDrawing::TextAlignment::Trailing);
	nativeEdit->SetText(text.c_str());

	nativeEdit->ShowAsync(&onTextEntryCompeteEvent);
}

void EditWidget::OnTextEnteredComplete(int32_t result)
{
	if (result == gmpi::MP_OK)
	{
		MpString s;
		nativeEdit->GetText(s.getUnknown());

		SetText(s.str());

		OnChangedEvent(s.str().c_str());
	}

	nativeEdit = nullptr; // release it.
}