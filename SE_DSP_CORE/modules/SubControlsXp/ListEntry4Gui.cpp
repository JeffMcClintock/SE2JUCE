#include "./ListEntry4Gui.h"
#include <string>
#include <algorithm>
#include "../se_sdk3/it_enum_list.h"
#include "../shared/unicode_conversion.h"

using namespace gmpi;
using namespace gmpi_gui;
using namespace JmUnicodeConversions;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, ListEntry4Gui, L"SE List Entry4" );
SE_DECLARE_INIT_STATIC_FILE(ListEntry4_Gui);

ListEntry4Gui::ListEntry4Gui()
{
	// initialise pins.
	initializePin(0, pinChoice, static_cast<MpGuiBaseMemberPtr2>(&ListEntry4Gui::redraw) );
	initializePin(1, pinItemList, static_cast<MpGuiBaseMemberPtr2>(&ListEntry4Gui::redraw));
	initializePin(2, pinStyle, static_cast<MpGuiBaseMemberPtr2>(&ListEntry4Gui::onSetStyle) );
	initializePin(3, pinWriteable );
	initializePin(4, pinGreyed, static_cast<MpGuiBaseMemberPtr2>(&ListEntry4Gui::redraw) );
	initializePin(5, pinHint );
	initializePin(6, pinMenuItems );
	initializePin(7, pinMenuSelection );
}

int32_t ListEntry4Gui::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	if (!getCapture())
		return gmpi::MP_UNHANDLED;

	releaseCapture();

	GmpiGui::GraphicsHost host(getGuiHost());
	nativeMenu = host.createPlatformMenu(Point(0, 0));

	it_enum_list itr(pinItemList);

	for (itr.First(); !itr.IsDone(); itr.Next())
	{
		int32_t flags = itr.CurrentItem()->value == pinChoice ? gmpi_gui::MP_PLATFORM_MENU_TICKED : 0;

		auto& txt = itr.CurrentItem()->text;
		auto itemValue = itr.CurrentItem()->value;

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

		nativeMenu.AddItem(txt, itemValue, flags);
	}

	nativeMenu.ShowAsync(
		[this](int32_t result) -> void
		{
			this->OnPopupmenuComplete(result);
		}
	);

	return gmpi::MP_OK;
}

int32_t ListEntry4Gui::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	// Let host handle right-clicks.
	if ((flags & GG_POINTER_FLAG_FIRSTBUTTON) == 0)
	{
		return gmpi::MP_OK; // Indicate successful hit, so right-click menu can show.
	}

	if (pinWriteable == false)
		return gmpi::MP_UNHANDLED;

	setCapture();

	return gmpi::MP_HANDLED;
}

std::string ListEntry4Gui::getDisplayText()
{
	it_enum_list itr(pinItemList);

	for (itr.First(); !itr.IsDone(); itr.Next())
	{
		enum_entry* e = itr.CurrentItem();

		if (e->value != pinChoice)
			continue;

		// Special commands (sub-menus)?
		auto& txt = itr.CurrentItem()->text;
		if (txt.size() > 3)
		{
			int i;
			for (i = 1; i < 4; ++i)
			{
				if (txt[0] != txt[i])
					break;
			}

			if (i == 4) // First 4 chars the same? Ignore.
			{
				continue;
			}
		}

		return WStringToUtf8(txt);
	}

	return "";
}

void ListEntry4Gui::OnPopupmenuComplete(int32_t result)
{
	if (result == gmpi::MP_OK)
	{
		pinChoice = nativeMenu.GetSelectedId();
		invalidateRect();
	}

	nativeMenu.setNull(); // release it.
}

