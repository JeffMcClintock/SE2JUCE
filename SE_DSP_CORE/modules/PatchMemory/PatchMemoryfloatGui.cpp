// prevent MS CPP - "'swprintf' was declared deprecated" warning.
#define _CRT_SECURE_NO_DEPRECATE 
#pragma warning(disable : 4996)

#include "PatchMemoryFloatGui.h"

REGISTER_GUI_PLUGIN( PatchMemoryFloatGui, L"SE PatchMemory Float" );
SE_DECLARE_INIT_STATIC_FILE(PatchMemoryFloat_Gui);
REGISTER_GUI_PLUGIN(PatchMemoryFloatGui, L"SE PatchMemory Float Latchable");
SE_DECLARE_INIT_STATIC_FILE(PatchMemoryFloatLatchable_Gui);

#define INIT_PINB( pinId, name ) initializePin( pinId, pin##name, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryFloatGui::on##name##Changed ) );

PatchMemoryFloatGui::PatchMemoryFloatGui(IMpUnknown* host) : MpGuiBase(host)
{
	INIT_PINB(0, ValueIn);
	INIT_PINB(1, NameIn);
	INIT_PINB(2, MenuItemsIn);
	INIT_PINB(3, MenuSelectionIn);
	INIT_PINB(4, MouseDownIn);
	INIT_PINB(5, AnimationIn);

	INIT_PINB(6, Name);
	INIT_PINB(7, Value);
	INIT_PINB(8, AnimationPosition);
	INIT_PINB(9, MenuItems);
	INIT_PINB(10, MenuSelection);
	INIT_PINB(11, MouseDown);
}

void PatchMemoryFloatGui::onValueInChanged()
{
	pinValue = pinValueIn;
}

void PatchMemoryFloatGui::onValueChanged()
{
	pinValueIn = pinValue;
}

void PatchMemoryFloatGui::onAnimationInChanged()
{
	pinAnimationPosition = pinAnimationIn;
}

void PatchMemoryFloatGui::onAnimationPositionChanged()
{
	pinAnimationIn = pinAnimationPosition;
}

void PatchMemoryFloatGui::onNameInChanged()
{
	pinName = pinNameIn;
}

void PatchMemoryFloatGui::onNameChanged()
{
	pinNameIn = pinName;
}

void PatchMemoryFloatGui::onMenuItemsInChanged()
{
	pinMenuItems = pinMenuItemsIn;
}

void PatchMemoryFloatGui::onMenuSelectionInChanged()
{
	pinMenuSelection = pinMenuSelectionIn;
}

void PatchMemoryFloatGui::onMenuItemsChanged()
{
	pinMenuItemsIn = pinMenuItems;
}

void PatchMemoryFloatGui::onMenuSelectionChanged()
{
	pinMenuSelectionIn = pinMenuSelection;
}

void PatchMemoryFloatGui::onMouseDownChanged()
{
	pinMouseDownIn = pinMouseDown;
}

void PatchMemoryFloatGui::onMouseDownInChanged()
{
	pinMouseDown = pinMouseDownIn;
}
