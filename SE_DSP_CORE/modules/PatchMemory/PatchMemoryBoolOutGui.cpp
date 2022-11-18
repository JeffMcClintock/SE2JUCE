#include "PatchMemoryBoolOutGui.h"

REGISTER_GUI_PLUGIN( PatchMemoryBoolOutGui, L"SE PatchMemory Bool Out" );
SE_DECLARE_INIT_STATIC_FILE(PatchMemoryBoolOut_Gui);

// handy macro to save typing.
#define INIT_PINB( pinId, name ) initializePin( pinId, pin##name, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryBoolOutGui::on##name##Changed ) );

PatchMemoryBoolOutGui::PatchMemoryBoolOutGui( IMpUnknown* host ) : MpGuiBase( host )
{
	INIT_PINB(0, ValueIn);
	INIT_PINB(1, NameIn);
	INIT_PINB(2, MenuItemsIn);
	INIT_PINB(3, MenuSelectionIn);
	INIT_PINB(4, AnimationIn);

	INIT_PINB(5, Name);
	INIT_PINB(6, Value);

	// old pinAnimationPosition.initialize( this, 8 ); // empty callback.
	initializePin( 8, pinAnimationPosition ); // empty callback.

	INIT_PINB(9, MenuItems);
	INIT_PINB(10, MenuSelection);
}

void PatchMemoryBoolOutGui::onValueInChanged()
{
	pinValue = pinValueIn;
}

void PatchMemoryBoolOutGui::onValueChanged()
{
	pinValueIn = pinValue;
}

void PatchMemoryBoolOutGui::onAnimationInChanged()
{
	pinAnimationPosition = pinAnimationIn;
}

void PatchMemoryBoolOutGui::onNameInChanged()
{
	pinName = pinNameIn;
}

void PatchMemoryBoolOutGui::onNameChanged()
{
	pinNameIn = pinName;
}

void PatchMemoryBoolOutGui::onMenuItemsInChanged()
{
	pinMenuItems = pinMenuItemsIn;
}

void PatchMemoryBoolOutGui::onMenuSelectionInChanged()
{
	pinMenuSelection = pinMenuSelectionIn;
}

void PatchMemoryBoolOutGui::onMenuItemsChanged()
{
	pinMenuItemsIn = pinMenuItems;
}

void PatchMemoryBoolOutGui::onMenuSelectionChanged()
{
	pinMenuSelectionIn = pinMenuSelection;
}

