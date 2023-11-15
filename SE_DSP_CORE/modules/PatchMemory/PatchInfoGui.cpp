#include "./PatchInfoGui.h"

REGISTER_GUI_PLUGIN( PatchInfoGui, L"SE Patch Info" );
SE_DECLARE_INIT_STATIC_FILE(PatchInfo_Gui)

PatchInfoGui::PatchInfoGui(IMpUnknown* host) : MpGuiBase(host)
{
	// initialise pins.
	initializePin( 0, programIn, static_cast<MpGuiBaseMemberPtr>( &PatchInfoGui::onSetprogramIn ) );
	initializePin( 1, programNamesListIn, static_cast<MpGuiBaseMemberPtr>( &PatchInfoGui::onSetprogramNamesListIn ) );
	initializePin( 2, programNameIn, static_cast<MpGuiBaseMemberPtr>( &PatchInfoGui::onSetprogramNameIn ) );
	initializePin( 3, midiChannelIn, static_cast<MpGuiBaseMemberPtr>( &PatchInfoGui::onSetmidiChannelIn ) );
	initializePin( 4, patchCommandIn );

	// outputs to user controls.
	initializePin( 5, programOut, static_cast<MpGuiBaseMemberPtr>( &PatchInfoGui::onSetprogramOut ) );
	initializePin( 6, programNamesListOut );
	initializePin( 7, programNameOut, static_cast<MpGuiBaseMemberPtr>( &PatchInfoGui::onSetprogramNameOut ) );
	initializePin( 8, midiChannelOut, static_cast<MpGuiBaseMemberPtr>( &PatchInfoGui::onSetchannelOut ) );
	initializePin( 9, channelListOut );
	initializePin( 10, patchCommandOut, static_cast<MpGuiBaseMemberPtr>( &PatchInfoGui::onSetpatchCommandOut ) );
	initializePin( 11, patchCommandListOut );

	initializePin( 12, patchCommandListIn, static_cast<MpGuiBaseMemberPtr>( &PatchInfoGui::onSetPatchCommandListIn ) );
}

int32_t PatchInfoGui::initialize()
{
	channelListOut = L"All=-1,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16";
//	patchCommandListOut = L",Copy Patch,Load Inst,Save Inst,Load Bank,Save Bank";

	return gmpi::MP_OK;
}

enum GuiHostCommands{ HC_null, HC_CopyPatch, HC_LoadPatch, HC_SavePatch, HC_LoadBank, HC_SaveBank };

// handle pin updates
void PatchInfoGui::onSetProgram()
{
	programIn = programOut;
}

void PatchInfoGui::onSetprogramIn()
{
	programOut = programIn;
}

void PatchInfoGui::onSetprogramNameIn()
{
	programNameOut = programNameIn;
}
void PatchInfoGui::onSetprogramNameOut()
{
	programNameIn = programNameOut;
}

void PatchInfoGui::onSetprogramNamesListIn()
{
	programNamesListOut = programNamesListIn;
}

void PatchInfoGui::onSetmidiChannelIn()
{
	midiChannelOut = midiChannelIn;
}

void PatchInfoGui::onSetprogramOut()
{
	programIn = programOut;
}

void PatchInfoGui::onSetchannelOut()
{
	midiChannelIn = midiChannelOut;
}

void PatchInfoGui::onSetpatchCommandOut()
{
	patchCommandIn = patchCommandOut;

	// Reset to Zero after executing command.
	if( patchCommandOut > 0 )
	{
		patchCommandOut = 0;
		patchCommandIn = 0;
	}
}

void PatchInfoGui::onSetPatchCommandListIn()
{
	patchCommandListOut = patchCommandListIn;
}
