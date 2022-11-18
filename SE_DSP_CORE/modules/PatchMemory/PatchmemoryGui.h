#ifndef PATCHMEMORYGUI_H_INCLUDED
#define PATCHMEMORYGUI_H_INCLUDED

#include "mp_sdk_gui.h"

class PatchmemoryGui : public MpGuiBase
{
public:
	PatchmemoryGui( IMpUnknown* host );

	// overrides

private:
 	void onSetValueIn();
 	void onSetNameIn();
 	void onSetMenuItems();
 	void onSetMenuSelection();
 	void onSetMouseDown();
 	void onSetAnimationIn();
 	void onSetName();
 	void onSetValue();
 	void onSetAnimationPosition();
 	void onSetMenuItemsOut();
 	void onSetMenuSelectionOut();
 	void onSetMouseDownOut();
 	FloatGuiPin pinValueIn;
 	StringGuiPin pinNameIn;
 	StringGuiPin pinMenuItems;
 	IntGuiPin pinMenuSelection;
 	BoolGuiPin pinMouseDown;
 	FloatGuiPin pinAnimationIn;
 	StringGuiPin pinName;
 	FloatGuiPin pinValue;
 	FloatGuiPin pinAnimationPosition;
 	StringGuiPin pinMenuItemsOut;
 	IntGuiPin pinMenuSelectionOut;
 	BoolGuiPin pinMouseDownOut;
};

#endif


