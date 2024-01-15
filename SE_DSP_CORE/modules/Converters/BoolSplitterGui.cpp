#include "./BoolSplitterGui.h"

using namespace gmpi;
using namespace gmpi_gui;

REGISTER_GUI_PLUGIN( BoolSplitterGui, L"SE Bool Splitter" );
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, FloatSplitterGui, L"SE Float Splitter");


BoolSplitterGui::BoolSplitterGui( IMpUnknown* host ) : MpGuiBase(host)
,pinCount_(0)
{
	// initialise pins.
	pinOut.initialize(this, 0, static_cast<MpGuiBaseMemberPtr>(&BoolSplitterGui::onSetOutput));
}

int32_t BoolSplitterGui::initialize()
{
	getHost()->getPinCount(pinCount_);

	return MpGuiBase::initialize();
}

// handle pin updates.
void BoolSplitterGui::onSetOutput()
{
	bool boolPinValue = pinOut;
	for (int i = 0; i < pinCount_; ++i)
	{
		getHost()->pinTransmit(i, sizeof(boolPinValue), &boolPinValue);
	}
}

int32_t BoolSplitterGui::setPin(int32_t pinId, int32_t voice, int32_t size, void* data)
{
	int32_t r = MpGuiBase::setPin(pinId, voice, size, data);

	if (pinId > 0)
	{
		bool boolPinValue;
		VariableFromRaw<bool>(size, data, boolPinValue);
		pinOut = boolPinValue;

		onSetOutput();
	}

	return r;
}
