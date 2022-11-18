#include "PatchMemoryBlobOutGui.h"

REGISTER_GUI_PLUGIN( PatchMemoryBlobOutGui, L"SE PatchMemory Blob Out" );

// handy macro to save typing.
#define INIT_PINB( pinId, name ) initializePin( pinId, pin##name, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryBlobOutGui::on##name##Changed ) );

PatchMemoryBlobOutGui::PatchMemoryBlobOutGui( IMpUnknown* host ) : MpGuiBase( host )
{
	INIT_PINB(0, ValueIn);
	INIT_PINB(1, NameIn);
	INIT_PINB(2, MenuItemsIn);
	INIT_PINB(3, MenuSelectionIn);
	INIT_PINB(4, Name);
	INIT_PINB(5, Value);
	INIT_PINB(6, MenuItems);
	INIT_PINB(7, MenuSelection);
}

void PatchMemoryBlobOutGui::onValueInChanged()
{
	pinValue = pinValueIn;
}

void PatchMemoryBlobOutGui::onValueChanged()
{
	pinValueIn = pinValue;
}

void PatchMemoryBlobOutGui::onNameInChanged()
{
	pinName = pinNameIn;
}

void PatchMemoryBlobOutGui::onNameChanged()
{
	pinNameIn = pinName;
}

void PatchMemoryBlobOutGui::onMenuItemsInChanged()
{
	pinMenuItems = pinMenuItemsIn;
}

void PatchMemoryBlobOutGui::onMenuSelectionInChanged()
{
	pinMenuSelection = pinMenuSelectionIn;
}

void PatchMemoryBlobOutGui::onMenuItemsChanged()
{
	pinMenuItemsIn = pinMenuItems;
}

void PatchMemoryBlobOutGui::onMenuSelectionChanged()
{
	pinMenuSelectionIn = pinMenuSelection;
}

