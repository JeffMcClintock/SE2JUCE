// prevent MS CPP - "'swprintf' was declared deprecated" warning.
#define _CRT_SECURE_NO_DEPRECATE 
#pragma warning(disable : 4996)

#include "PatchMemoryBoolGui.h"

REGISTER_GUI_PLUGIN( PatchMemoryBoolGui, L"SE PatchMemory Bool" );
SE_DECLARE_INIT_STATIC_FILE(PatchMemoryBoolGui);

#define INIT_PINB( pinId, name ) initializePin( pinId, pin##name, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryBoolGui::on##name##Changed ) );

PatchMemoryBoolGui::PatchMemoryBoolGui(IMpUnknown* host) : MpGuiBase(host)
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

void PatchMemoryBoolGui::onValueInChanged()
{
	pinValue = pinValueIn;
}

void PatchMemoryBoolGui::onValueChanged()
{
	pinValueIn = pinValue;
}

void PatchMemoryBoolGui::onAnimationInChanged()
{
	if(!inhibitFeedback) // prevent jitter
	{
		pinAnimationPosition = pinAnimationIn;
	}
}

void PatchMemoryBoolGui::onAnimationPositionChanged()
{
	inhibitFeedback = true;
	pinAnimationIn = pinAnimationPosition;
	inhibitFeedback = false;
}

void PatchMemoryBoolGui::onNameInChanged()
{
	pinName = pinNameIn;
}

void PatchMemoryBoolGui::onNameChanged()
{
	pinNameIn = pinName;
}

void PatchMemoryBoolGui::onMenuItemsInChanged()
{
	pinMenuItems = pinMenuItemsIn;
}

void PatchMemoryBoolGui::onMenuSelectionInChanged()
{
	pinMenuSelection = pinMenuSelectionIn;
}

void PatchMemoryBoolGui::onMenuItemsChanged()
{
	pinMenuItemsIn = pinMenuItems;
}

void PatchMemoryBoolGui::onMenuSelectionChanged()
{
	pinMenuSelectionIn = pinMenuSelection;
}

void PatchMemoryBoolGui::onMouseDownChanged()
{
	pinMouseDownIn = pinMouseDown;
}

void PatchMemoryBoolGui::onMouseDownInChanged()
{
	pinMouseDown = pinMouseDownIn;
}
