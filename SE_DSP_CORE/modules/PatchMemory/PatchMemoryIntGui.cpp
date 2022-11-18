#include "PatchMemoryIntGui.h"

REGISTER_GUI_PLUGIN( PatchMemoryIntGui, L"SE PatchMemory Int" );

#define INIT_PINB( pinId, name ) initializePin( pinId, pin##name, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryIntGui::on##name##Changed ) );

PatchMemoryIntGui::PatchMemoryIntGui(IMpUnknown* host) : MpGuiBase(host)
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

void PatchMemoryIntGui::onValueInChanged()
{
	pinValue = pinValueIn;
}

void PatchMemoryIntGui::onValueChanged()
{
	pinValueIn = pinValue;
}

void PatchMemoryIntGui::onAnimationInChanged()
{
	if(!inhibitFeedback) // prevent jitter
	{
		pinAnimationPosition = pinAnimationIn;
	}
}

void PatchMemoryIntGui::onAnimationPositionChanged()
{
	inhibitFeedback = true;
	pinAnimationIn = pinAnimationPosition;
	inhibitFeedback = false;
}

void PatchMemoryIntGui::onNameInChanged()
{
	pinName = pinNameIn;
}

void PatchMemoryIntGui::onNameChanged()
{
	pinNameIn = pinName;
}

void PatchMemoryIntGui::onMenuItemsInChanged()
{
	pinMenuItems = pinMenuItemsIn;
}

void PatchMemoryIntGui::onMenuSelectionInChanged()
{
	pinMenuSelection = pinMenuSelectionIn;
}

void PatchMemoryIntGui::onMenuItemsChanged()
{
	pinMenuItemsIn = pinMenuItems;
}

void PatchMemoryIntGui::onMenuSelectionChanged()
{
	pinMenuSelectionIn = pinMenuSelection;
}

void PatchMemoryIntGui::onMouseDownChanged()
{
	pinMouseDownIn = pinMouseDown;
}

void PatchMemoryIntGui::onMouseDownInChanged()
{
	pinMouseDown = pinMouseDownIn;
}
