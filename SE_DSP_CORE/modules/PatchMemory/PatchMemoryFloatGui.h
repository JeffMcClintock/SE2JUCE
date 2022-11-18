#ifndef PatchMemoryFloatGui_H_INCLUDED
#define PatchMemoryFloatGui_H_INCLUDED

#include "mp_sdk_gui.h"

class PatchMemoryFloatGui :
	public MpGuiBase
{
public:
	PatchMemoryFloatGui(IMpUnknown* host);

	FloatGuiPin		pinValueIn;
	StringGuiPin	pinNameIn;
	StringGuiPin	pinMenuItemsIn;
	IntGuiPin		pinMenuSelectionIn; // Out really.
	FloatGuiPin		pinAnimationIn;
	BoolGuiPin		pinMouseDownIn;

	StringGuiPin	pinName;
	FloatGuiPin		pinValue;
	FloatGuiPin		pinAnimationPosition;
	StringGuiPin	pinMenuItems;
	IntGuiPin		pinMenuSelection;
	BoolGuiPin		pinMouseDown;

private:
	void onValueInChanged();
	void onValueChanged();
	void onNameInChanged();
	void onNameChanged();
	void onAnimationPositionChanged();
	void onMouseDownChanged();
	void onAnimationInChanged();
	void onMouseDownInChanged();
	void onMenuItemsInChanged();
	void onMenuSelectionInChanged();
	void onMenuItemsChanged();
	void onMenuSelectionChanged();
};

#endif