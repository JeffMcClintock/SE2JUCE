#include "./TriggeratorGui.h"


REGISTER_GUI_PLUGIN( TriggeratorGui, L"SE Triggerator" );

TriggeratorGui::TriggeratorGui( IMpUnknown* host ) : MpGuiBase(host)
	,lastButtonDown(false)
{
	// initialise pins.
	Reset.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&TriggeratorGui::onSetBoolVal) );
	boolVal.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&TriggeratorGui::onSetBoolVal) );
	floatVal.initialize( this, 2, static_cast<MpGuiBaseMemberPtr>(&TriggeratorGui::onSetFloatVal) );
}

// handle pin updates.
void TriggeratorGui::onSetBoolVal()
{
	floatVal = (float) boolVal;
}

void TriggeratorGui::onSetFloatVal()
{
	bool buttonDown = floatVal > 0.0f;

	if( !buttonDown )// button just released.
	{
		if( lastButtonDown ) // and was previously pushed down.
		{
			// Pulse bool pin.
			boolVal = true;
			boolVal = false;

			// Pulse Reset pin.
			Reset = true;
			Reset = false;
		}
	}

	lastButtonDown = buttonDown;
}

