#ifndef PatchMemoryBlobGui_H_INCLUDED
#define PatchMemoryBlobGui_H_INCLUDED

#include "mp_sdk_gui.h"

class PatchMemoryBlobGui :
	public MpGuiBase
{
public:
	PatchMemoryBlobGui(IMpUnknown* host);

	BlobGuiPin		pinValueIn;
	StringGuiPin	pinNameIn;
	StringGuiPin	pinTypeIn;
	StringGuiPin	pinMenuItemsIn;
	IntGuiPin		pinMenuSelectionIn; // Out really.

	StringGuiPin	pinName;
	BlobGuiPin		pinValue;
	StringGuiPin	pinType;
	StringGuiPin	pinMenuItems;
	IntGuiPin		pinMenuSelection;
private:
	void onValueInChanged();
	void onValueChanged();
	void onNameInChanged();
	void onNameChanged();
	void onTypeInChanged();
	void onMenuItemsInChanged();
	void onMenuSelectionInChanged();
	void onTypeChanged();
	void onMenuItemsChanged();
	void onMenuSelectionChanged();
};

#endif