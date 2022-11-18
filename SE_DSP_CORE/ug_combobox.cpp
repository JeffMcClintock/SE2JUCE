#include "pch.h"
#include "ug_combobox.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_combobox)

namespace
{
REGISTER_MODULE_1_BC(7,L"List Entry", IDS_MN_LIST_ENTRY,IDS_MG_CONTROLS, ug_combobox ,CF_STRUCTURE_VIEW|CF_PANEL_VIEW,L"This control is for connecting to a 'List of Values' type plug, (green).  For example connect one of these to an Oscillator's 'Waveform' input.");
}

#define PN_CHOICE 6
#define PLG_PATCH_VALUE 9

short appearance; // only here to satisfy  LIST_VAR3  macro

void ug_combobox::ListInterface2(InterfaceObjectArray& PList)
{
	//////////////////////////// ug_control::ListInterface2(PList);
	LIST_VAR3( L"Channel", trash_sample_ptr, DR_IN, DT_ENUM , L"-1", MIDI_CHAN_LIST,IO_DISABLE_IF_POS|IO_IGNORE_PATCH_CHANGE|IO_POLYPHONIC_ACTIVE, L"MIDI Channel");
	// these two retained only as dummy connections to MIDI automator (to ensure correct Sort Order)
	LIST_VAR3N( L"MIDI In", DR_IN, DT_MIDI2 , L"0", L"", IO_DISABLE_IF_POS, L"");
	LIST_VAR3N( L"MIDI Out", DR_OUT, DT_MIDI2 , L"", L"", IO_DISABLE_IF_POS, L"");
	LIST_VAR3( L"MIDI Controller ID", controller_number, DR_IN, DT_ENUM , L"-1",L"", IO_POLYPHONIC_ACTIVE|IO_HIDE_PIN, L"");
	LIST_VAR3( L"Ignore Program Change", ignore_prog_change, DR_IN, DT_BOOL , L"0", L"", IO_HIDE_PIN, L"");
	LIST_VAR3( L"MIDI NRPN", nrpn, DR_IN, DT_ENUM , L"0", L"range 0,16255",IO_HIDE_PIN, L"");
	////////////////////////////

	LIST_VAR3( L"Choice", EnumOut, DR_OUT, DT_ENUM , L"", L"", IO_AUTOCONFIGURE_PARAMETER, L"Connect this to any input needing a list of values");
	LIST_VAR3( L"Appearance",appearance, DR_IN,DT_ENUM, L"0", L"Combo Box=0, LED Stack, Labeled LED Stack, Selector, Button Stack, Rotary Switch, Rotary (No Text), Up/Down Select", IO_MINIMISED, L"Sets the appearance of the control" );
	LIST_VAR3( L"Show Title On Panel", trash_bool_ptr, DR_IN, DT_BOOL , L"1", L"", IO_MINIMISED, L"Leaves control's title blank on Control Panel");
	// SE 1.1
	LIST_VAR3( L"patchValue", patchValue_, DR_IN, DT_ENUM , L"0", L"", IO_HIDE_PIN|IO_PATCH_STORE, L"");
	LIST_VAR_UI( L"patchValue", DR_OUT, DT_ENUM , L"", L"", IO_UI_COMMUNICATION|IO_HIDE_PIN|IO_PATCH_STORE, L""); // not counted in dsp
	LIST_VAR_UI( L"Hint", DR_OUT, DT_TEXT , L"", L"", IO_UI_COMMUNICATION|IO_MINIMISED, L""); // not counted in dsp
}

//, Rotary Switch
ug_combobox::ug_combobox() :
	EnumOut(0)
{
	// drum synths get voice numbers propagated back upstream, cause engine to
	// suspend controls along with voice. However that will cause crash when vst host trys to automate suspended control
	// better solution would be to keep all controls in voice zero!!
	// SetFlag(UGF_NEVER_SUSPEND);
}

int ug_combobox::Open()
{
	ug_control::Open();
	//done by ug_control		patch _control_container->RegAutomatedParameter(this);
	SET_CUR_FUNC( &ug_base::process_sleep );
	return 0;
}

void ug_combobox::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state )
{
	if( p_to_plug == GetPlug( PLG_PATCH_VALUE ) )
	{
		EnumOut = patchValue_;
		// OutputChange( SampleClock(), GetPlug(PN_CHOICE), ST_ONE_OFF );
		SendPinsCurrentValue( SampleClock(), GetPlug(PN_CHOICE) );
	}
}

