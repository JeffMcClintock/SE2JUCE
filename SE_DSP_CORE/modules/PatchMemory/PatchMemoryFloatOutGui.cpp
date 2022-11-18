#include "PatchMemoryFloatOutGui.h"

// This module replaces two older modules.  Difference is one has a 'Value' in pin, other don't (it's hidden so I can share the code).
REGISTER_GUI_PLUGIN( PatchMemoryFloatOutGui, L"SE PatchMemory Float Out" );
REGISTER_GUI_PLUGIN( PatchMemoryFloatOutGui, L"SE PatchMemory Float Out B2" );
SE_DECLARE_INIT_STATIC_FILE(PatchMemoryFloatOut_Gui);

// handy macro to save typing.
#define INIT_PINB( pinId, name ) initializePin( pinId, pin##name, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryFloatOutGui::on##name##Changed ) );

PatchMemoryFloatOutGui::PatchMemoryFloatOutGui( IMpUnknown* host ) : MpGuiBase( host )
{
	INIT_PINB(0, ValueIn);
	INIT_PINB(1, NameIn);
	INIT_PINB(2, MenuItemsIn);
	INIT_PINB(3, MenuSelectionIn);
	INIT_PINB(4, AnimationIn);

	INIT_PINB(5, Name);
	INIT_PINB(6, Value);

	// old pinValueBackwardCompatible.initialize( this, 7 ); // empty callback.
	// old pinAnimationPosition.initialize( this, 8 ); // empty callback.

	initializePin( 7, pinValueBackwardCompatible );
	initializePin( 8, pinAnimationPosition );

	INIT_PINB(9, MenuItems);
	INIT_PINB(10, MenuSelection);
}

void PatchMemoryFloatOutGui::onValueInChanged()
{
	pinValue = pinValueIn;
	pinValueBackwardCompatible = pinValueIn;
}

void PatchMemoryFloatOutGui::onValueChanged()
{
	pinValueIn = pinValue;
}

void PatchMemoryFloatOutGui::onAnimationInChanged()
{
	pinAnimationPosition = pinAnimationIn;
}

void PatchMemoryFloatOutGui::onNameInChanged()
{
	pinName = pinNameIn;
}

void PatchMemoryFloatOutGui::onNameChanged()
{
	pinNameIn = pinName;
}

void PatchMemoryFloatOutGui::onMenuItemsInChanged()
{
	pinMenuItems = pinMenuItemsIn;
}

void PatchMemoryFloatOutGui::onMenuSelectionInChanged()
{
	pinMenuSelection = pinMenuSelectionIn;
}

void PatchMemoryFloatOutGui::onMenuItemsChanged()
{
	pinMenuItemsIn = pinMenuItems;
}

void PatchMemoryFloatOutGui::onMenuSelectionChanged()
{
	pinMenuSelectionIn = pinMenuSelection;
}

