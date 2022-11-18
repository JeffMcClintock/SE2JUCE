#include "./PatchmemoryGui.h"


REGISTER_GUI_PLUGIN( PatchmemoryGui, L"SE PatchMemory" );

PatchmemoryGui::PatchmemoryGui( IMpUnknown* host ) : MpGuiBase(host)
{
	// initialise pins.
	pinValueIn.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetValueIn) );
	pinNameIn.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetNameIn) );
	pinMenuItems.initialize( this, 2, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetMenuItems) );
	pinMenuSelection.initialize( this, 3, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetMenuSelection) );
	pinMouseDown.initialize( this, 4, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetMouseDown) );
	pinAnimationIn.initialize( this, 5, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetAnimationIn) );
	pinName.initialize( this, 6, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetName) );
	pinValue.initialize( this, 7, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetValue) );
	pinAnimationPosition.initialize( this, 8, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetAnimationPosition) );
	pinMenuItemsOut.initialize( this, 9, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetMenuItemsOut) );
	pinMenuSelectionOut.initialize( this, 10, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetMenuSelectionOut) );
	pinMouseDownOut.initialize( this, 11, static_cast<MpGuiBaseMemberPtr>(&PatchmemoryGui::onSetMouseDownOut) );
}

// handle pin updates.
void PatchmemoryGui::onSetValueIn()
{
	// pinValueIn changed
}

void PatchmemoryGui::onSetNameIn()
{
	// pinNameIn changed
}

void PatchmemoryGui::onSetMenuItems()
{
	// pinMenuItems changed
}

void PatchmemoryGui::onSetMenuSelection()
{
	// pinMenuSelection changed
}

void PatchmemoryGui::onSetMouseDown()
{
	// pinMouseDown changed
}

void PatchmemoryGui::onSetAnimationIn()
{
	// pinAnimationIn changed
}

void PatchmemoryGui::onSetName()
{
	// pinName changed
}

void PatchmemoryGui::onSetValue()
{
	// pinValue changed
}

void PatchmemoryGui::onSetAnimationPosition()
{
	// pinAnimationPosition changed
}

void PatchmemoryGui::onSetMenuItemsOut()
{
	// pinMenuItemsOut changed
}

void PatchmemoryGui::onSetMenuSelectionOut()
{
	// pinMenuSelectionOut changed
}

void PatchmemoryGui::onSetMouseDownOut()
{
	// pinMouseDownOut changed
}

