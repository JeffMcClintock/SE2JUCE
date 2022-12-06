#include "pch.h"
#include "ug_text_entry.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_text_entry)


namespace
{
REGISTER_MODULE_1(L"Text Entry", IDS_MN_TEXT_ENTRY,IDS_MG_CONTROLS,ug_text_entry ,CF_STRUCTURE_VIEW|CF_PANEL_VIEW,L"This control is for connecting to a Text Values' type plug, (red).  For example connect one of these to a Wave Out's 'Filename' input.");
}

#define PLG_TEXT_OUT 6
#define PLG_PATCH_VALUE 8

// Fill an array of InterfaceObjects with plugs and parameters
void ug_text_entry::ListInterface2(InterfaceObjectArray& PList)
{
	//////////////////////////// ug_control::ListInterface2(PList);
	LIST_VAR3( L"Channel", trash_sample_ptr, DR_IN, DT_ENUM , L"-1", MIDI_CHAN_LIST,IO_DISABLE_IF_POS|IO_IGNORE_PATCH_CHANGE|IO_POLYPHONIC_ACTIVE, L"MIDI Channel");
	// these two retained only as dummy connections to MIDI automator (to ensure correct Sort Order)
	LIST_VAR3N( L"MIDI In",  DR_IN, DT_MIDI2 , L"0", L"", IO_DISABLE_IF_POS, L"");
	LIST_VAR3N(L"MIDI Out", DR_OUT, DT_MIDI2 , L"", L"", IO_DISABLE_IF_POS, L"");
	LIST_VAR3( L"MIDI Controller ID", controller_number, DR_IN, DT_ENUM , L"-1",L"", IO_POLYPHONIC_ACTIVE|IO_HIDE_PIN, L"");
	LIST_VAR3( L"Ignore Program Change", ignore_prog_change, DR_IN, DT_BOOL , L"0", L"", IO_HIDE_PIN, L"");
	LIST_VAR3( L"MIDI NRPN", nrpn, DR_IN, DT_ENUM , L"0", L"range 0,16255",IO_HIDE_PIN, L"");
	////////////////////////////

	LIST_VAR3( L"Text Out", TextOut, DR_OUT, DT_TEXT , L"0", L"", IO_AUTOCONFIGURE_PARAMETER, L"");

	LIST_VAR3( L"Show Title On Panel", trash_bool_ptr, DR_IN, DT_BOOL , L"1", L"", IO_MINIMISED, L"Leaves control's title blank on Control Panel");
	// SE 1.1
	LIST_VAR3( L"patchValue", patchValue_, DR_IN, DT_TEXT , L"0", L"", IO_HIDE_PIN|IO_PATCH_STORE, L"");
	LIST_VAR_UI( L"patchValue", DR_OUT, DT_TEXT , L"", L"", IO_UI_COMMUNICATION|IO_HIDE_PIN|IO_PATCH_STORE, L""); // not counted in dsp
	LIST_VAR_UI( L"Hint", DR_OUT, DT_TEXT , L"", L"", IO_UI_COMMUNICATION|IO_MINIMISED, L""); // not counted in dsp
}

ug_text_entry::ug_text_entry() : ug_control()
{
}

void ug_text_entry::onSetPin(timestamp_t /*p_clock*/, UPlug* p_to_plug, state_type /*p_state*/ )
{
	if( p_to_plug == GetPlug( PLG_PATCH_VALUE ) )
	{
		TextOut = patchValue_;
		SendPinsCurrentValue( SampleClock(), GetPlug(PLG_TEXT_OUT) );
	}
}
