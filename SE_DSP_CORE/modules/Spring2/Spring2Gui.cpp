#include "./Spring2Gui.h"

REGISTER_GUI_PLUGIN( Spring2Gui, L"SE Spring2" );
REGISTER_GUI_PLUGIN( Spring2Gui, L"SE Spring" );

Spring2Gui::Spring2Gui(IMpUnknown* host) : MpGuiBase(host)
,prevMouseDown( false )
{
	// initialise pins
	normalisedValue.initialize( this, 0 );
	mouseDown.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&Spring2Gui::onChanged) );
	resetValue.initialize(this, 2);
	enable.initialize(this, 3, static_cast<MpGuiBaseMemberPtr>(&Spring2Gui::onChanged));
}

void Spring2Gui::onChanged()
{
	// mouseDown changed
	if(enable && mouseDown == false && prevMouseDown == true )
	{
		normalisedValue = resetValue;
	}

	prevMouseDown = mouseDown;
}

// uses a pass-though for mouse-down, to avoid wheel springing back while animating from MIDI side
class Spring3Gui final : public SeGuiInvisibleBase
{
	void onSetMouseDownIn()
	{
		pinMouseDownOut = pinMouseDownIn;
	}

	void onSetMouseDownOut()
	{
		pinMouseDownIn = pinMouseDownOut;
		onChanged();
	}

	void onChanged()
	{
		if (pinOnOff && pinMouseDownOut == false)
		{
			pinNormalisedValue = pinResetValue;
		}
	}

	FloatGuiPin pinNormalisedValue;
	BoolGuiPin pinMouseDownIn;
	BoolGuiPin pinMouseDownOut;
	FloatGuiPin pinResetValue;
	BoolGuiPin pinOnOff;

public:
	Spring3Gui()
	{
		initializePin(pinNormalisedValue);
		initializePin(pinMouseDownIn, static_cast<MpGuiBaseMemberPtr2>(&Spring3Gui::onSetMouseDownIn));
		initializePin(pinMouseDownOut, static_cast<MpGuiBaseMemberPtr2>(&Spring3Gui::onSetMouseDownOut));
		initializePin(pinResetValue);
		initializePin(pinOnOff, static_cast<MpGuiBaseMemberPtr2>(&Spring3Gui::onChanged));
	}
};

namespace
{
	auto r = gmpi::Register<Spring3Gui>::withId(L"SE Spring3");
}
