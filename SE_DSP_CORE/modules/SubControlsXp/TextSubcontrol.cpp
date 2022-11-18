#include "TextSubcontrol.h"
#include "../shared/unicode_conversion.h"

using namespace gmpi;
using namespace GmpiDrawing;
using namespace JmUnicodeConversions;

int32_t TextSubcontrol::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext)
{
	GmpiDrawing::Graphics g(drawingContext);

	auto r = getRect();
	Rect textRect(r);
	textRect.Deflate((float)border, (float)border);

	auto textformat = GetTextFormat(pinStyle);
	auto textdata = GetFontMetatdata(pinStyle);

	// Background Fill. Currently fills behind frame line too (could be more efficient).
	auto brush = g.CreateSolidColorBrush(textdata->getBackgroundColor());
	g.FillRectangle(textRect, brush);

	if (pinGreyed == true)
		brush.SetColor(Color::Gray);
	else
		brush.SetColor(textdata->getColor());

	if(textdata->verticalSnapBackwardCompatibilityMode)
	{
		auto directXOffset = textdata->getLegacyVerticalOffset();
		textRect.top += directXOffset;
		textRect.bottom += directXOffset;
	}

	g.DrawTextU(getDisplayText(), textformat, textRect, brush, (int32_t)DrawTextOptions::Clip);

	return gmpi::MP_OK;
}

void TextSubcontrol::onSetStyle()
{
	invalidateRect();
	getGuiHost()->invalidateMeasure();
}

int32_t TextSubcontrol::populateContextMenu(float x, float y, gmpi::IMpUnknown* contextMenuItemsSink)
{
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

int32_t TextSubcontrol::onContextMenu(int32_t selection)
{
	pinMenuSelection = selection; // send menu momentarily, then reset.
	pinMenuSelection = -1;

	return gmpi::MP_OK;
}

int32_t TextSubcontrol::getToolTip(float x, float y, gmpi::IMpUnknown * returnToolTipString)
{
	IString* returnValue = 0;

	auto hint = pinHint.getValue();

	if (hint.empty() || MP_OK != returnToolTipString->queryInterface(gmpi::MP_IID_RETURNSTRING, reinterpret_cast<void**>(&returnValue)))
	{
		return gmpi::MP_NOSUPPORT;
	}

	auto utf8ToolTip = WStringToUtf8(hint);

	returnValue->setData(utf8ToolTip.data(), (int32_t)utf8ToolTip.size());

	return gmpi::MP_OK;
}

int32_t TextSubcontrol::measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize)
{
	*returnDesiredSize = availableSize;
	const float minSize = 4;
	returnDesiredSize->height = (std::max)(returnDesiredSize->height, minSize);
	returnDesiredSize->width = (std::max)(returnDesiredSize->width, minSize);

	return gmpi::MP_OK;
}
