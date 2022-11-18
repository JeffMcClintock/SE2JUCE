#ifndef PatchMemoryBlobOutGui_H_INCLUDED
#define PatchMemoryBlobOutGui_H_INCLUDED

#include "mp_sdk_gui.h"

class PatchMemoryBlobOutGui :
	public MpGuiBase
{
public:
	PatchMemoryBlobOutGui(IMpUnknown* host);

	BlobGuiPin		pinValueIn;
	StringGuiPin	pinNameIn;
	StringGuiPin	pinMenuItemsIn;
	IntGuiPin		pinMenuSelectionIn; // Out really.

	StringGuiPin	pinName;
	BlobGuiPin		pinValue;
	StringGuiPin	pinMenuItems;
	IntGuiPin		pinMenuSelection;

private:
	void onValueInChanged();
	void onValueChanged();
	void onNameInChanged();
	void onNameChanged();
	void onMenuItemsInChanged();
	void onMenuSelectionInChanged();
	void onMenuItemsChanged();
	void onMenuSelectionChanged();
};

#endif