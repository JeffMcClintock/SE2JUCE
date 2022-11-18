// prevent MS CPP - 'swprintf' was declared deprecated warning
#define _CRT_SECURE_NO_DEPRECATE 
#pragma warning(disable : 4996)

#include "PatchMemoryTextGui.h"

REGISTER_GUI_PLUGIN( PatchMemoryTextGui, L"SE PatchMemory Text2" );

#define INIT_PINB( pinId, name ) initializePin( pinId, pin##name, static_cast<MpGuiBaseMemberPtr>( &PatchMemoryTextGui::on##name##Changed ) );

PatchMemoryTextGui::PatchMemoryTextGui(IMpUnknown* host) : MpGuiBase(host)
{
	INIT_PINB(0, ValueIn);
	INIT_PINB(1, NameIn);
	INIT_PINB(2, FileExtensionIn);
//	INIT_PINB(3, RangeMaximimIn);
	INIT_PINB(3, MenuItemsIn);
	INIT_PINB(4, MenuSelectionIn);

	INIT_PINB(5, Name);
	INIT_PINB(6, Value);
//	INIT_PINB(8, AnimationPosition);
	INIT_PINB(7, FileExtension);
//	INIT_PINB(10, RangeMaximim);
	INIT_PINB(8, MenuItems);
	INIT_PINB(9, MenuSelection);
}

void PatchMemoryTextGui::onValueInChanged()
{
	pinValue = pinValueIn;

	// above does not trigger notification on pinValue, so..
	onValueChanged();
}

void PatchMemoryTextGui::onValueChanged()
{
	pinValueIn = pinValue;
}

void PatchMemoryTextGui::onNameInChanged()
{
	pinName = pinNameIn;
}

void PatchMemoryTextGui::onNameChanged()
{
	pinNameIn = pinName;
}

void PatchMemoryTextGui::onFileExtensionInChanged()
{
	pinFileExtension = pinFileExtensionIn;
}

void PatchMemoryTextGui::onMenuItemsInChanged()
{
	pinMenuItems = pinMenuItemsIn;
}

void PatchMemoryTextGui::onMenuSelectionInChanged()
{
	pinMenuSelection = pinMenuSelectionIn;
}

void PatchMemoryTextGui::onFileExtensionChanged()
{
	pinFileExtensionIn = pinFileExtension;
}

void PatchMemoryTextGui::onMenuItemsChanged()
{
	pinMenuItemsIn = pinMenuItems;
}

void PatchMemoryTextGui::onMenuSelectionChanged()
{
	pinMenuSelectionIn = pinMenuSelection;
}

