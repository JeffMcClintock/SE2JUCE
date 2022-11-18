#ifndef PATCHINFOGUI_H_INCLUDED
#define PATCHINFOGUI_H_INCLUDED

#include "mp_sdk_gui.h"

class PatchInfoGui : public MpGuiBase
{
public:
	PatchInfoGui( IMpUnknown* host );
	virtual int32_t MP_STDCALL initialize();

private:
	void onSetProgram();
	void onSetprogramIn();
	void onSetprogramNameIn();
	void onSetCategoryIn();
	void onSetprogramNamesListIn();
	void onSetmidiChannelIn();
	void onSetprogramOut();
	void onSetprogramNameOut();
	void onSetCategoryOut();
	void onSetchannelOut();
	void onSetPatchCommandListIn();
	void onSetpatchCommandOut();

	// inputs from host.
 	IntGuiPin programIn;
 	StringGuiPin programNameIn;
 	StringGuiPin programNamesListIn;
 	IntGuiPin midiChannelIn;
 	IntGuiPin patchCommandIn;

    // outputs to user controls.
	IntGuiPin programOut;
 	StringGuiPin programNameOut;
 	StringGuiPin programNamesListOut;
 	IntGuiPin midiChannelOut;
 	StringGuiPin channelListOut;
 	IntGuiPin patchCommandOut;
 	StringGuiPin patchCommandListIn;
 	StringGuiPin patchCommandListOut;
};

#endif