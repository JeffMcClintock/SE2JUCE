// prevent MS CPP - 'swprintf' was declared deprecated warning
#define _CRT_SECURE_NO_DEPRECATE 
#pragma warning(disable : 4996)

#include "PatchMemoryBlobGui.h"

REGISTER_GUI_PLUGIN( PatchMemoryBlobGui, L"SE PatchMemory Blob" );

#define INIT_PINB( pinId, name ) initializePin( pinId, pin##name, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryBlobGui::on##name##Changed ) );

PatchMemoryBlobGui::PatchMemoryBlobGui(IMpUnknown* host) : MpGuiBase(host)
{
	INIT_PINB(0, ValueIn);
	INIT_PINB(1, NameIn);
	INIT_PINB(2, TypeIn);
	INIT_PINB(3, MenuItemsIn);
	INIT_PINB(4, MenuSelectionIn);

	INIT_PINB(5, Name);
	INIT_PINB(6, Value);
	INIT_PINB(7, Type);
	INIT_PINB(8, MenuItems);
	INIT_PINB(9, MenuSelection);
}

void PatchMemoryBlobGui::onValueInChanged()
{
	pinValue = pinValueIn;

	// above does not trigger notification on pinValue, so..
	onValueChanged();
}

void PatchMemoryBlobGui::onValueChanged()
{
	pinValueIn = pinValue;
}

void PatchMemoryBlobGui::onNameInChanged()
{
	pinName = pinNameIn;
}

void PatchMemoryBlobGui::onNameChanged()
{
	pinNameIn = pinName;
}

void PatchMemoryBlobGui::onTypeInChanged()
{
	pinType = pinTypeIn;
}

void PatchMemoryBlobGui::onMenuItemsInChanged()
{
	pinMenuItems = pinMenuItemsIn;
}

void PatchMemoryBlobGui::onMenuSelectionInChanged()
{
	pinMenuSelection = pinMenuSelectionIn;
}

void PatchMemoryBlobGui::onTypeChanged()
{
	pinTypeIn = pinType;
}

void PatchMemoryBlobGui::onMenuItemsChanged()
{
	pinMenuItemsIn = pinMenuItems;
}

void PatchMemoryBlobGui::onMenuSelectionChanged()
{
	pinMenuSelectionIn = pinMenuSelection;
}

