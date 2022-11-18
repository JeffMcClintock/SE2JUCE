#include "PatchMemoryIntOutGui.h"

REGISTER_GUI_PLUGIN( PatchMemoryIntOutGui, L"SE PatchMemory Int Out" );

// handy macro to save typing.
#define INIT_PINB( name ) initializePin( pin##name, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryIntOutGui::on##name##Changed ) );

PatchMemoryIntOutGui::PatchMemoryIntOutGui( IMpUnknown* host ) : MpGuiBase( host )
{
	INIT_PINB( ValueIn);
	INIT_PINB( NameIn);
	INIT_PINB( MenuItemsIn);
	INIT_PINB( MenuSelectionIn);
	INIT_PINB( AnimationIn);

	INIT_PINB( Name);
	INIT_PINB( Value);

	//INIT_PINB( MenuItems);
	//INIT_PINB( MenuSelection);

	initializePin(8, pinAnimationPosition); // Skip non-existant pin 7. Empty callback on purpose. It's an output only.
	initializePin( pinMenuItems);
	//initializePin(10, pinMenuSelection, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryIntOutGui::onMenuSelectionChanged ));
	INIT_PINB( MenuSelection);
}


void PatchMemoryIntOutGui::onValueInChanged()
{
	pinValue = pinValueIn;
}

void PatchMemoryIntOutGui::onValueChanged()
{
	pinValueIn = pinValue;
}

void PatchMemoryIntOutGui::onAnimationInChanged()
{
	pinAnimationPosition = pinAnimationIn;
}

void PatchMemoryIntOutGui::onNameInChanged()
{
	pinName = pinNameIn;
}

void PatchMemoryIntOutGui::onNameChanged()
{
	pinNameIn = pinName;
}

void PatchMemoryIntOutGui::onMenuItemsInChanged()
{
	pinMenuItems = pinMenuItemsIn;
}

void PatchMemoryIntOutGui::onMenuSelectionInChanged()
{
	pinMenuSelection = pinMenuSelectionIn;
}

void PatchMemoryIntOutGui::onMenuItemsChanged()
{
	pinMenuItemsIn = pinMenuItems;
}

void PatchMemoryIntOutGui::onMenuSelectionChanged()
{
	pinMenuSelectionIn = pinMenuSelection;
}

