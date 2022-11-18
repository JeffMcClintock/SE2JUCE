#ifndef PatchMemoryListGui_H_INCLUDED
#define PatchMemoryListGui_H_INCLUDED

#include "mp_sdk_gui.h"

class PatchMemoryListGui :
	public MpGuiBase
{
public:
	PatchMemoryListGui(IMpUnknown* host);

	IntGuiPin		pinValueIn;
	StringGuiPin	pinNameIn;
	StringGuiPin	pinListValuesIn;
	StringGuiPin	pinMenuItemsIn;
	IntGuiPin		pinMenuSelectionIn; // Out really.
	BoolGuiPin		pinMouseDownIn;

	StringGuiPin	pinName;
	IntGuiPin		pinValue;
	StringGuiPin	pinListValues;
	StringGuiPin	pinMenuItems;
	IntGuiPin		pinMenuSelection;
	BoolGuiPin		pinMouseDown;

private:
	void onValueInChanged();
	void onValueChanged();
	void onNameInChanged();
	void onNameChanged();
	void onMouseDownChanged();
	void onMouseDownInChanged();
	void onListValuesInChanged();
	void onMenuItemsInChanged();
	void onMenuSelectionInChanged();
	void onListValuesChanged();
	void onMenuItemsChanged();
	void onMenuSelectionChanged();
};

#endif