#include "./SubpatchManagerGui.h"

using namespace gmpi;
using namespace gmpi_gui;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, SubpatchManagerGui, L"SE Sub-Patch Manager");

SubpatchManagerGui::SubpatchManagerGui()
{
	// initialise pins.
	initializePin(pinProgram, static_cast<MpGuiBaseMemberPtr2>(&SubpatchManagerGui::onSetProgram));
	initializePin(pinProgramNamesList, static_cast<MpGuiBaseMemberPtr2>(&SubpatchManagerGui::onSetProgramNamesList));
	initializePin(pinProgramName, static_cast<MpGuiBaseMemberPtr2>(&SubpatchManagerGui::onSetProgramName));
	initializePin(pinPatchCommand, static_cast<MpGuiBaseMemberPtr2>(&SubpatchManagerGui::onSetPatchCommand));
	initializePin(pinPatchCommandList, static_cast<MpGuiBaseMemberPtr2>(&SubpatchManagerGui::onSetPatchCommandList));

	initializePin(pinProgramOut, static_cast<MpGuiBaseMemberPtr2>(&SubpatchManagerGui::onSetProgramOut));
	initializePin(pinProgramNamesListOut, static_cast<MpGuiBaseMemberPtr2>(&SubpatchManagerGui::onSetProgramNamesListOut));
	initializePin(pinProgramNameOut, static_cast<MpGuiBaseMemberPtr2>(&SubpatchManagerGui::onSetProgramNameOut));
	initializePin(pinPatchCommandOut, static_cast<MpGuiBaseMemberPtr2>(&SubpatchManagerGui::onSetPatchCommandOut));
	initializePin(pinPatchCommandsList);
	initializePin(pinStrictParameterMatch);
}

// handle pin updates.
void SubpatchManagerGui::onSetProgram()
{
	// pinProgram changed
}

void SubpatchManagerGui::onSetProgramNamesList()
{
	// pinProgramNamesList changed
}

void SubpatchManagerGui::onSetProgramName()
{
	// pinProgramName changed
}

void SubpatchManagerGui::onSetPatchCommand()
{
	// pinPatchCommand changed
}

void SubpatchManagerGui::onSetProgramOut()
{
	// pinProgramOut changed
}

void SubpatchManagerGui::onSetProgramNamesListOut()
{
	// pinProgramNamesListOut changed
}

void SubpatchManagerGui::onSetProgramNameOut()
{
	// pinProgramNameOut changed
}

void SubpatchManagerGui::onSetPatchCommandOut()
{
	int commandId = pinPatchCommandOut;

	if (pinStrictParameterMatch && commandId == 21)
	{
		commandId = 22;
	}

	pinPatchCommand = commandId;

	// Reset to Zero after executing command.
	if (pinPatchCommandOut > 0)
	{
		pinPatchCommandOut = 0;
		pinPatchCommand = 0;
	}
}

void SubpatchManagerGui::onSetPatchCommandList()
{
	pinPatchCommandsList = pinPatchCommandList;
}

