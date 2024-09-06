#include "mp_sdk_gui2.h"
#include "../se_sdk3/mp_gui.h"
#include "../se_sdk3/Drawing.h"
#include "../shared/FontCache.h"
#include "../shared/unicode_conversion.h"

using namespace gmpi;
using namespace gmpi_gui;
using namespace GmpiDrawing;
using namespace JmUnicodeConversions;

class ListboxGui : public MpGuiGfxBase, public FontCacheClient
{
protected:
//	static const int border = 3;
	static const int scrollbar_w = 10;

	IntGuiPin pinChoice;
	StringGuiPin pinItemList;
	StringGuiPin pinStyle;
	BoolGuiPin pinWriteable;
	BoolGuiPin pinGreyed;
	StringGuiPin pinHint;
	StringGuiPin pinMenuItems;
	IntGuiPin pinMenuSelection;

	float rowheight = 1.0f;
	bool isScrolling = false;
	int scrollItemIndex = 0;
	int totalItems = 0;
	int visibleItems = 0;
	GmpiDrawing_API::MP1_POINT prevPoint;
	float cumulativeScroll = {};

public:
	ListboxGui()
	{
		// initialise pins.
		initializePin(0, pinChoice, static_cast<MpGuiBaseMemberPtr2>(&ListboxGui::onSetValue));
		initializePin(1, pinItemList, static_cast<MpGuiBaseMemberPtr2>(&ListboxGui::onSetItemList));
		initializePin(2, pinStyle, static_cast<MpGuiBaseMemberPtr2>(&ListboxGui::onSetStyle));
		initializePin(3, pinWriteable);
		initializePin(4, pinGreyed, static_cast<MpGuiBaseMemberPtr2>(&ListboxGui::redraw));
		initializePin(5, pinHint);
		initializePin(6, pinMenuItems);
		initializePin(7, pinMenuSelection);
	}

	void onSetStyle()
	{
		getGuiHost()->invalidateMeasure();
	}

	void redraw()
	{
		invalidateRect();
	}

	void onSetItemList()
	{
		calcScrollBars();
		invalidateRect();
	}

	void onSetValue()
	{
		it_enum_list itr(pinItemList);
		itr.FindValue(pinChoice);
		if (!itr.IsDone())
		{
			const int newIndex = itr.CurrentItem()->index;

			if (newIndex < scrollItemIndex)
			{
				scrollItemIndex = newIndex;
			}

			else if (newIndex > scrollItemIndex + visibleItems - 2)
			{
				scrollItemIndex = newIndex - visibleItems + 1;
			}

			const auto maxScroll = (std::max)(0, totalItems - visibleItems);
			scrollItemIndex = std::clamp(scrollItemIndex, 0, maxScroll);
		}
		invalidateRect();
	}

	void calcScrollBars()
	{
		auto r = getRect();

		it_enum_list itr(pinItemList);
		totalItems = itr.size();

		auto textFormat = GetTextFormat(pinStyle);
		GmpiDrawing_API::MP1_FONT_METRICS fontMetrics;
		textFormat.GetFontMetrics(&fontMetrics);
		const int padding = 1;
		rowheight = fontMetrics.ascent + fontMetrics.descent + fontMetrics.lineGap + 2 * padding;

		visibleItems = (int) (r.getHeight() / rowheight );

		auto maxScroll = (std::max)(0, totalItems - visibleItems);
		scrollItemIndex = (std::min)(scrollItemIndex, maxScroll);
	}

	virtual int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);
		auto r = getRect();

		auto textFormat = GetTextFormat(pinStyle);
		auto brush = g.CreateSolidColorBrush(GetFontMetatdata(pinStyle)->getColor());
		auto BackgroundSelectedBrush = g.CreateSolidColorBrush(Color::White);
		auto scrollbarBrush = g.CreateSolidColorBrush(Color::LightGray);
		
		auto directXOffset = GetFontMetatdata(pinStyle)->getLegacyVerticalOffset();

		Rect clipRect{ 0, 0, r.getWidth() - scrollbar_w, r.getHeight() };
		g.PushAxisAlignedClip(clipRect);

		Rect textRect{ 0.0f, 0.0f, clipRect.getWidth(), rowheight };

		it_enum_list itr(pinItemList);
		int i = 0;
		for (itr.First(); !itr.IsDone(); itr.Next())
		{
			if(itr.CurrentItem()->getType() != enum_entry_type::Normal)
			{
				continue;
			}

			if (scrollItemIndex <= i)
			{
					if (textRect.bottom > r.getHeight())
						break;

					if (itr.CurrentItem()->value == pinChoice)
					{
						g.FillRectangle(textRect, BackgroundSelectedBrush);
					}

					g.DrawTextU(WStringToUtf8(itr.CurrentItem()->text), textFormat, textRect, brush, (int32_t)DrawTextOptions::Clip);

					textRect.Offset(0.0f, rowheight);
			}
			++i;
		}

		clipRect.left = clipRect.right;
		clipRect.right = r.getWidth();

		g.PopAxisAlignedClip();

		// scroll-bar
		float scrollbarLengthFrac = visibleItems / (float)totalItems;
		if (scrollbarLengthFrac < 1.0f) // hide scroll bar if not needed.
		{
			float scrollbarLength = scrollbarLengthFrac * r.getHeight();
			float scrollbarStart = scrollItemIndex * r.getHeight() / totalItems;
			Rect scrollbarRect{ r.getWidth() - scrollbar_w, scrollbarStart, r.getWidth(), scrollbarStart + scrollbarLength };
			g.FillRectangle(scrollbarRect, scrollbarBrush);
		}
		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		// Let host handle right-clicks.
		if ((flags & GG_POINTER_FLAG_FIRSTBUTTON) == 0)
		{
			return gmpi::MP_OK; // Indicate successful hit, so right-click menu can show.
		}

		setCapture();

		isScrolling = point.x > getRect().getWidth() - scrollbar_w;
		prevPoint = point;
		cumulativeScroll = {};

		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		if (getCapture() && isScrolling)
		{
			doScroll(point.y - prevPoint.y);
			prevPoint = point;
		}
		return gmpi::MP_OK;
	}

	void doScroll(float dy)
	{
		cumulativeScroll += dy;

		const auto pixelsScrollPerRow = rowheight;

		auto newscrollItemIndex = scrollItemIndex + (int)(cumulativeScroll / pixelsScrollPerRow);
		newscrollItemIndex = std::clamp(newscrollItemIndex, 0, std::max(0, totalItems - visibleItems));

		if (newscrollItemIndex != scrollItemIndex)
		{
			cumulativeScroll -= (newscrollItemIndex - scrollItemIndex) * pixelsScrollPerRow;
			scrollItemIndex = newscrollItemIndex;
			invalidateRect();
		}
	}

	int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		if (getCapture())
		{
			releaseCapture();
			if (!isScrolling)
			{
				auto selectedIndex = scrollItemIndex + (int)(point.y / rowheight);
				it_enum_list itr(pinItemList);
				int i = 0;
				for (itr.First(); !itr.IsDone(); itr.Next())
				{
					if(itr.CurrentItem()->getType() != enum_entry_type::Normal)
					{
						continue;
					}

					if (selectedIndex == i)
					{
						pinChoice = itr.CurrentItem()->value;
						break;
					}
					++i;
				}
			}
		}
		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL onMouseWheel(int32_t flags, int32_t delta, GmpiDrawing_API::MP1_POINT point) override
	{
		// ignore horizontal scrolling
		if (0 != (flags & gmpi_gui_api::GG_POINTER_KEY_SHIFT))
			return gmpi::MP_UNHANDLED;

		const float scale = (flags & gmpi_gui_api::GG_POINTER_KEY_CONTROL) ? 1.0f / 12.0f : 1.0f / 1.2f;
		doScroll(-delta * scale);

		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL populateContextMenu(float x, float y, gmpi::IMpUnknown* contextMenuItemsSink) override
	{
		gmpi::IMpContextItemSink* sink;
		contextMenuItemsSink->queryInterface(gmpi::MP_IID_CONTEXT_ITEMS_SINK, reinterpret_cast<void**>(&sink));

		it_enum_list itr(pinMenuItems);

		for (itr.First(); !itr.IsDone(); ++itr)
		{
			sink->AddItem(WStringToUtf8(itr.CurrentItem()->text).c_str(), itr.CurrentItem()->value);
		}
		return gmpi::MP_OK;
	}

	virtual int32_t MP_STDCALL onContextMenu(int32_t selection) override
	{
		pinMenuSelection = selection; // send menu momentarily, then reset.
		pinMenuSelection = -1;

		return gmpi::MP_OK;
	}

	// Old way.
	virtual int32_t MP_STDCALL getToolTip(float x, float y, gmpi::IMpUnknown* returnToolTipString) override
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

	virtual int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override
	{
		*returnDesiredSize = availableSize;
		const float minSize = 4;
		returnDesiredSize->height = (std::max)(returnDesiredSize->height, minSize);
		returnDesiredSize->width = (std::max)(returnDesiredSize->width, minSize);
		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL arrange(GmpiDrawing_API::MP1_RECT finalRect) override
	{
		auto r = MpGuiGfxBase::arrange(finalRect);

		calcScrollBars();

		return r;
	}

	// new way. IMpGraphics2.
	int32_t MP_STDCALL getToolTip(GmpiDrawing_API::MP1_POINT point, gmpi::IString* returnString) override
	{
		auto utf8String = (std::string)pinHint;
		returnString->setData(utf8String.data(), (int32_t) utf8String.size());
		return gmpi::MP_OK;
	}
};

namespace
{
	bool r = Register<ListboxGui>::withId(L"SE Listbox");
}
