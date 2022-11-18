#include "PatchMemoryTextOutGui.h"

REGISTER_GUI_PLUGIN( PatchMemoryTextOutGui, L"SE PatchMemory Text Out" );
SE_DECLARE_INIT_STATIC_FILE(PatchMemoryTextOut_Gui);

// handy macro to save typing.
#define INIT_PINB( pinId, name ) initializePin( pinId, pin##name, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryTextOutGui::on##name##Changed ) );

PatchMemoryTextOutGui::PatchMemoryTextOutGui( IMpUnknown* host ) : MpGuiBase( host )
{
	INIT_PINB(0, ValueIn);
	INIT_PINB(1, NameIn);
	INIT_PINB(2, MenuItemsIn);
	INIT_PINB(3, MenuSelectionIn);
	INIT_PINB(5, Name);
	INIT_PINB(6, Value);
	INIT_PINB(9, MenuItems);
	INIT_PINB(10, MenuSelection);
}

void PatchMemoryTextOutGui::onValueInChanged()
{
	pinValue = pinValueIn;
}

void PatchMemoryTextOutGui::onValueChanged()
{
	pinValueIn = pinValue;
}

void PatchMemoryTextOutGui::onNameInChanged()
{
	pinName = pinNameIn;
}

void PatchMemoryTextOutGui::onNameChanged()
{
	pinNameIn = pinName;
}

void PatchMemoryTextOutGui::onMenuItemsInChanged()
{
	pinMenuItems = pinMenuItemsIn;
}

void PatchMemoryTextOutGui::onMenuSelectionInChanged()
{
	pinMenuSelection = pinMenuSelectionIn;
}

void PatchMemoryTextOutGui::onMenuItemsChanged()
{
	pinMenuItemsIn = pinMenuItems;
}

void PatchMemoryTextOutGui::onMenuSelectionChanged()
{
	pinMenuSelectionIn = pinMenuSelection;
}

