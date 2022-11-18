#ifndef SUBPATCHMANAGERGUI_H_INCLUDED
#define SUBPATCHMANAGERGUI_H_INCLUDED

#include "mp_sdk_gui2.h"

class SubpatchManagerGui : public gmpi_gui::MpGuiInvisibleBase
{
public:
	SubpatchManagerGui();

private:
	void onSetProgram();
	void onSetProgramNamesList();
	void onSetProgramName();
	void onSetPatchCommand();
	void onSetProgramOut();
	void onSetProgramNamesListOut();
	void onSetProgramNameOut();
	void onSetPatchCommandOut();
	void onSetPatchCommandList();

	IntGuiPin pinProgram;
	StringGuiPin pinProgramNamesList;
	StringGuiPin pinProgramName;
	IntGuiPin pinPatchCommand;
	IntGuiPin pinProgramOut;
	StringGuiPin pinProgramNamesListOut;
	StringGuiPin pinProgramNameOut;
	IntGuiPin pinPatchCommandOut;
	StringGuiPin pinPatchCommandsList;
	StringGuiPin pinPatchCommandList;
	BoolGuiPin pinStrictParameterMatch;
};

#endif


