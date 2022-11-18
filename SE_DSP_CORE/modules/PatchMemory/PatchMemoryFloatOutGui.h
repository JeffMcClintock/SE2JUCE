#ifndef PatchMemoryFloatOutGui_H_INCLUDED
#define PatchMemoryFloatOutGui_H_INCLUDED

#include "mp_sdk_gui.h"

class PatchMemoryFloatOutGui :
	public MpGuiBase
{
public:
	PatchMemoryFloatOutGui(IMpUnknown* host);

	FloatGuiPin		pinValueIn;
	StringGuiPin	pinNameIn;
	StringGuiPin	pinMenuItemsIn;
	IntGuiPin		pinMenuSelectionIn; // Out really.

	StringGuiPin	pinName;
	FloatGuiPin		pinValue;
	FloatGuiPin		pinValueBackwardCompatible;
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