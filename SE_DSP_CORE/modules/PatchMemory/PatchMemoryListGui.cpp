// prevent MS CPP - 'swprintf' was declared deprecated warning
#define _CRT_SECURE_NO_DEPRECATE 
#pragma warning(disable : 4996)

#include "PatchMemoryListGui.h"
#include "../se_sdk3/it_enum_list.h"

REGISTER_GUI_PLUGIN( PatchMemoryListGui, L"SE PatchMemory List3" );
SE_DECLARE_INIT_STATIC_FILE(PatchMemoryList3_Gui)

#define INIT_PINB( pinId, name ) initializePin( pinId, pin##name, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryListGui::on##name##Changed ) );

PatchMemoryListGui::PatchMemoryListGui(IMpUnknown* host) : MpGuiBase(host)
{
	INIT_PINB(0, ValueIn);
	INIT_PINB(1, NameIn);
	INIT_PINB(2, ListValuesIn);
	INIT_PINB(3, MenuItemsIn);
	INIT_PINB(4, MenuSelectionIn);
	INIT_PINB(5, MouseDownIn);

	INIT_PINB(6, Name);
	INIT_PINB(7, Value);
	INIT_PINB(8, ListValues);
	INIT_PINB(9, MenuItems);
	INIT_PINB(10, MenuSelection);
	INIT_PINB(11, MouseDown);
}

void PatchMemoryListGui::onValueInChanged()
{
	pinValue = pinValueIn;

	// above does not trigger notification on pinValue, so..
	onValueChanged();
}

void PatchMemoryListGui::onValueChanged()
{
	// quality control.
	it_enum_list it(pinListValuesIn);

	if (it.FindValue(pinValue))
	{
		pinValueIn = pinValue;
	}
}

void PatchMemoryListGui::onNameInChanged()
{
	pinName = pinNameIn;
}

void PatchMemoryListGui::onNameChanged()
{
	pinNameIn = pinName;
}

void PatchMemoryListGui::onListValuesInChanged()
{
	pinListValues = pinListValuesIn;
}

void PatchMemoryListGui::onMenuItemsInChanged()
{
	pinMenuItems = pinMenuItemsIn;
}

void PatchMemoryListGui::onMenuSelectionInChanged()
{
	pinMenuSelection = pinMenuSelectionIn;
}

void PatchMemoryListGui::onListValuesChanged()
{
	pinListValuesIn = pinListValues;
}

void PatchMemoryListGui::onMenuItemsChanged()
{
	pinMenuItemsIn = pinMenuItems;
}

void PatchMemoryListGui::onMenuSelectionChanged()
{
	pinMenuSelectionIn = pinMenuSelection;
}

void PatchMemoryListGui::onMouseDownChanged()
{
	pinMouseDownIn = pinMouseDown;
}

void PatchMemoryListGui::onMouseDownInChanged()
{
	pinMouseDown = pinMouseDownIn;
}
