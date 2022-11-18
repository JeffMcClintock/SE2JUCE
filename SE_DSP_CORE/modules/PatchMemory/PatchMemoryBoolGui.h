#ifndef PatchMemoryBoolGui_H_INCLUDED
#define PatchMemoryBoolGui_H_INCLUDED

#include "mp_sdk_gui.h"

class PatchMemoryBoolGui :
	public MpGuiBase
{
public:
	PatchMemoryBoolGui(IMpUnknown* host);

	BoolGuiPin		pinValueIn;
	StringGuiPin	pinNameIn;
	StringGuiPin	pinMenuItemsIn;
	IntGuiPin		pinMenuSelectionIn; // Out really.
	FloatGuiPin		pinAnimationIn;
	BoolGuiPin		pinMouseDownIn;

	StringGuiPin	pinName;
	BoolGuiPin		pinValue;
	FloatGuiPin		pinAnimationPosition;
	StringGuiPin	pinMenuItems;
	IntGuiPin		pinMenuSelection;
	BoolGuiPin		pinMouseDown;
	bool inhibitFeedback = {};

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