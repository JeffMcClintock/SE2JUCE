#ifndef PM_TEXT_GUI_H_INCLUDED
#define PM_TEXT_GUI_H_INCLUDED

#include "mp_sdk_gui.h"

class SampleLoader2Gui :
	public MpGuiBase
{
public:
	SampleLoader2Gui(IMpUnknown* host);

	StringGuiPin	pinFilename;
	IntGuiPin		pinBank;
	StringGuiPin	pinBankNames;
	StringGuiPin	pinPatchNames;
	IntGuiPin		pinPatch;

private:
	void onFileNameChanged();
};

#endif