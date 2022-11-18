#include "./BoolInverterGui.h"

REGISTER_GUI_PLUGIN( BoolInverterGui, L"SE GUI Bool Inverter" );
SE_DECLARE_INIT_STATIC_FILE(GUIBoolInverter_Gui);

BoolInverterGui::BoolInverterGui( IMpUnknown* host ) : MpGuiBase(host)
{
	// initialise pins.
	input.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&BoolInverterGui::onSetInput) );
	output.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&BoolInverterGui::onSetOutput) );
}

// Handle initial update (when input is default value).
int32_t BoolInverterGui::initialize()
{
	onSetInput();
	return MpGuiBase::initialize();
}

// handle pin updates.
void BoolInverterGui::onSetInput()
{
	output = !input;
}

void BoolInverterGui::onSetOutput()
{
	input = !output;
}
