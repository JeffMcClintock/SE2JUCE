#include "./FloatScaler2Gui.h"

SE_DECLARE_INIT_STATIC_FILE(FloatScaler2Gui);
REGISTER_GUI_PLUGIN( FloatScaler2Gui, L"SE Float Scaler2" );

FloatScaler2Gui::FloatScaler2Gui( IMpUnknown* host ) : MpGuiBase(host)
{
	// initialise pins.
	valueOut.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&FloatScaler2Gui::onSetValueOut) );
	multiplyby.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&FloatScaler2Gui::onSetValueIn) );
	add.initialize( this, 2, static_cast<MpGuiBaseMemberPtr>(&FloatScaler2Gui::onSetValueIn) );
	valueIn.initialize( this, 3, static_cast<MpGuiBaseMemberPtr>(&FloatScaler2Gui::onSetValueIn) );
}

// handle pin updates.
void FloatScaler2Gui::onSetValueOut()
{
	valueIn = (valueOut - add) / multiplyby;
}

void FloatScaler2Gui::onSetValueIn()
{
	valueOut = valueIn * multiplyby + add;
}

