#ifndef PatchMemoryFloatOutGui_H_INCLUDED
#define PatchMemoryFloatOutGui_H_INCLUDED

#include "mp_sdk_gui.h"

class PatchMemoryIntOutGui :
	public MpGuiBase
{
public:
	PatchMemoryIntOutGui(IMpUnknown* host);

	IntGuiPin		pinValueIn;
	StringGuiPin	pinNameIn;
	StringGuiPin	pinMenuItemsIn;
	IntGuiPin		pinMenuSelectionIn; // Out really.

	StringGuiPin	pinName;
	IntGuiPin		pinValue;
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