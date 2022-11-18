#ifndef PM_TEXT_GUI_H_INCLUDED
#define PM_TEXT_GUI_H_INCLUDED

#include "mp_sdk_gui.h"

#error no used. Remove from project

class PatchMemoryTextGui :
	public MpGuiBase
{
public:
	PatchMemoryTextGui(IMpUnknown* host);

	StringGuiPin	pinValueIn;
	StringGuiPin	pinNameIn;
	StringGuiPin	pinFileExtensionIn;
	StringGuiPin	pinMenuItemsIn;
	IntGuiPin		pinMenuSelectionIn; // Out really.

	StringGuiPin	pinName;
	StringGuiPin	pinValue;
	StringGuiPin	pinFileExtension;
	StringGuiPin	pinMenuItems;
	IntGuiPin		pinMenuSelection;
private:
	void onValueInChanged();
	void onValueChanged();
	void onNameInChanged();
	void onNameChanged();
	void onFileExtensionInChanged();
	void onMenuItemsInChanged();
	void onMenuSelectionInChanged();
	void onFileExtensionChanged();
	void onMenuItemsChanged();
	void onMenuSelectionChanged();
};

#endif