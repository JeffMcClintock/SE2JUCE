#ifndef PatchMemoryFloatOutGui_H_INCLUDED
#define PatchMemoryFloatOutGui_H_INCLUDED

#include "mp_sdk_gui.h"

class PatchMemoryBoolOutGui :
	public MpGuiBase
{
public:
	PatchMemoryBoolOutGui(IMpUnknown* host);

	BoolGuiPin		pinValueIn;
	StringGuiPin	pinNameIn;
	StringGuiPin	pinMenuItemsIn;
	IntGuiPin		pinMenuSelectionIn; // Out really.

	StringGuiPin	pinName;
	BoolGuiPin		pinValue;
	FloatGuiPin		pinAnimationIn;
	FloatGuiPin		pinAnimationPosition;
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