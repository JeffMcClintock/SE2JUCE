#ifndef PatchMemoryTextOutGui_H_INCLUDED
#define PatchMemoryTextOutGui_H_INCLUDED

#include "mp_sdk_gui.h"

class PatchMemoryTextOutGui :
	public MpGuiBase
{
public:
	PatchMemoryTextOutGui(IMpUnknown* host);

	StringGuiPin	pinValueIn;
	StringGuiPin	pinNameIn;
	StringGuiPin	pinMenuItemsIn;
	IntGuiPin		pinMenuSelectionIn; // Out really.

	StringGuiPin	pinName;
	StringGuiPin	pinValue;
	StringGuiPin	pinMenuItems;
	IntGuiPin		pinMenuSelection;

private:
	void onValueInChanged();
	void onValueChanged();
	void onNameInChanged();
	void onNameChanged();
	void onAnimationInChanged();
	void onMenuItemsInChanged();
	void onMenuSelectionInChanged();
	void onMenuItemsChanged();
	void onMenuSelectionChanged();
};

#endif