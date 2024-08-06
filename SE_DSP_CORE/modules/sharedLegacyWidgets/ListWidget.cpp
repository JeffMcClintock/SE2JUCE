#include "ListWidget.h"
#include <algorithm>
#include <math.h>
#include "../se_sdk3/MpString.h"
#include "../se_sdk3/it_enum_list.h"
#include "../shared/unicode_conversion.h"

using namespace gmpi;
using namespace gmpi_gui;
using namespace gmpi_sdk;
using namespace JmUnicodeConversions;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;

void ListWidget::OnRender(Graphics& g)
{
	auto r = position;

	// Background Fill.
	auto brush = g.CreateSolidColorBrush(Color::FromBytes(238, 238, 238));
	g.FillRectangle(r, brush);

	// Outline.
	Rect borderRect(r);
	borderRect.Deflate(0.5f);
	brush.SetColor(Color::FromBytes(150, 150, 150));
	g.DrawRectangle(borderRect, brush);

	// Drop-down indicator 'arrow'. Width twice height.
	float arrowHeight = floorf(r.getHeight() * 0.4f);
	float dropDownArrowLeft = r.right - arrowHeight - 4;
	brush.SetColor(Color::FromBytes(110, 110, 110));
	{
		const float penWidth = 2;

		auto geometry = g.GetFactory().CreatePathGeometry();
		auto sink = geometry.Open();

		Point p(borderRect.left + dropDownArrowLeft, borderRect.top + r.getHeight() * 0.5f - arrowHeight * 0.25f);

		sink.BeginFigure(p);
		p.x += arrowHeight * 0.5f;
		p.y += arrowHeight * 0.5f;
		sink.AddLine(p);
		p.x += arrowHeight * 0.5f;
		p.y -= arrowHeight * 0.5f;
		sink.AddLine(p);

		sink.EndFigure(FigureEnd::Open);
		sink.Close();
		g.DrawGeometry(geometry, brush, penWidth);
	}

	// Current selection text.
	brush.SetColor(Color::Black);// typeface_->getColor());

	static const float border = 3;
	Rect textRect(r);
	textRect.Deflate(border);
	textRect.right = dropDownArrowLeft - arrowHeight * 0.5f;
	textRect.top -= 1; // hack
/* no help
	// Note: SE sizes these too short to fit the text properly with ascenders and decenders.
	// Shift the rect up if text require more vertical space than avail.
	Size text_size;
	dtextFormat.GetTextExtentU("M", 1, text_size);
	float shift = (std::max)(0.0f, text_size.y - textRect.getHeight());
	textRect.top -= shift;
	textRect.bottom -= shift;
*/
	std::string text;
	it_enum_list itr(items);
	if (itr.FindValue(currentValue))
	{
		text = WStringToUtf8(itr.CurrentItem()->text);
	}

//	dtextFormat.SetParagraphAlignment(ParagraphAlignment::Near); // else overflowing text centres between lines.
	g.DrawTextU(text, dtextFormat, textRect, brush, GmpiDrawing_API::MP1_DRAW_TEXT_OPTIONS_CLIP);
}

void ListWidget::Init(const char* style)
{
	InitTextFormat(style, false);
}

GmpiDrawing::Size ListWidget::getSize()
{
	return Size(8.0f, static_cast<float>(typeface_->pixelHeight_) + 8);// +4); // + GetSystemMetrics(SM_CYEDGE) * 4 + hack.
}

bool ListWidget::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	// interested only in right-mouse clicks.
	return ((flags & (gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON | gmpi_gui_api::GG_POINTER_FLAG_NEW)) == (gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON | gmpi_gui_api::GG_POINTER_FLAG_NEW));
}

void ListWidget::OnPopupmenuComplete(int32_t result)
{
	if (result == gmpi::MP_OK)
	{
		auto returnSelectedId = nativeFileDialog.GetSelectedId();

		it_enum_list itr(items);
		itr.FindIndex(returnSelectedId);

		if (!itr.IsDone())
		{
			SetValue(itr.CurrentItem()->value);

			// Notify owner.
			OnChangedEvent(itr.CurrentItem()->value);
		}
	}

	nativeFileDialog.setNull(); // release it.
}

void ListWidget::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	GmpiGui::GraphicsHost host(getGuiHost());
	nativeFileDialog = host.createPlatformMenu(point);

	if(!nativeFileDialog) // currently not implemented on WINUI3
		return;

	it_enum_list itr(items);
	for (itr.First(); !itr.IsDone(); ++itr)
	{
		nativeFileDialog.AddItem(WStringToUtf8(itr.CurrentItem()->text).c_str(), itr.CurrentItem()->index);
	}

	nativeFileDialog.SetAlignment(GmpiDrawing::TextAlignment::Trailing);

	nativeFileDialog.ShowAsync(&onPopupMenuCompleteEvent);
}
