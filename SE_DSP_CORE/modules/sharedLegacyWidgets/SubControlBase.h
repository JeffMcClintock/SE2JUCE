#pragma once

#include "../se_sdk3/mp_sdk_gui2.h"
#include "../shared/it_enum_list.h"
#include "../shared/unicode_conversion.h"

class SubControlBase : public gmpi_gui::MpGuiGfxBase
{
protected:
	StringGuiPin pinMenuItems;
	IntGuiPin pinMenuSelection;
	BoolGuiPin pinMouseDown;
	StringGuiPin pinHint;

public:
	virtual int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		if (!getCapture())
		{
			return gmpi::MP_UNHANDLED;
		}

		releaseCapture();
		pinMouseDown = false;

		return gmpi::MP_OK;
	}

	virtual int32_t MP_STDCALL populateContextMenu(float x, float y, gmpi::IMpUnknown* contextMenuItemsSink) override
	{
		gmpi::IMpContextItemSink* sink = nullptr;
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

			sink->AddItem(JmUnicodeConversions::WStringToUtf8(itr.CurrentItem()->text).c_str(), itr.CurrentItem()->value, flags);
		}
		sink->release();
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
		gmpi::IString* returnString = nullptr;

		if (pinHint.getValue().empty() || gmpi::MP_OK != returnToolTipString->queryInterface(gmpi::MP_IID_RETURNSTRING, reinterpret_cast<void**>(&returnString)))
		{
			return gmpi::MP_NOSUPPORT;
		}

		auto utf8ToolTip = (std::string)pinHint;
		returnString->setData(utf8ToolTip.data(), (int32_t)utf8ToolTip.size());
		return gmpi::MP_OK;
	}

	// New way.
	int32_t MP_STDCALL getToolTip(GmpiDrawing_API::MP1_POINT point, gmpi::IString* returnString) override
	{
		auto utf8ToolTip = (std::string)pinHint;
		returnString->setData(utf8ToolTip.data(), static_cast< int32_t>(utf8ToolTip.size()));
		return gmpi::MP_OK;
	}
};


