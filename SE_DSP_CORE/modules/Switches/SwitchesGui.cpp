#include "./SwitchesGui.h"
#include <string>

using namespace gmpi;

typedef GuiSwitch<bool> GuiSwitchBool;
typedef GuiSwitch<float> GuiSwitchFloat;
typedef GuiSwitch<MpBlob> GuiSwitchBlob;
typedef GuiSwitch<int32_t> GuiSwitchInt;
typedef GuiSwitch<std::wstring> GuiSwitchText;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, GuiSwitchBool, L"SE GuiSwitch < (Bool)");
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, GuiSwitchInt, L"SE GuiSwitch < (Float)");
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, GuiSwitchBlob, L"SE GuiSwitch < (Blob)");
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, GuiSwitchInt, L"SE GuiSwitch < (Int)");
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, GuiSwitchText, L"SE GuiSwitch < (Text)");

/*
SwitchBoolGui::SwitchBoolGui()
{
	// initialise pins.
	initializePin(pinChoice);
	initializePin(pinValue, static_cast<MpGuiBaseMemberPtr2>(&SwitchBoolGui::onSetOutput));
}

void SwitchBoolGui::onSetOutput()
{
	if (pinChoice >= 0 && pinChoice < inValues.size())
	{
		bool boolPinValue = pinValue;
		int outputPinId = pinChoice + nonRepeatingPinCount;
		inValues[pinChoice] = boolPinValue;

		getHost()->pinTransmit(outputPinId, sizeof(boolPinValue), &boolPinValue);
	}
}

int32_t SwitchBoolGui::setPin(int32_t pinId, int32_t voice, int32_t size, const void* data)
{
	int32_t r = SeGuiInvisibleBase::setPin(pinId, voice, size, data);

	// Change on one Input pin.
	int inputIdx = pinId - nonRepeatingPinCount;
	if (inputIdx >= 0)
	{
		// In case we havn't counted pins yet.
		// May need to grow input value array to hold this value.
		while (inputIdx >= inValues.size())
		{
			inValues.push_back(false);
		}

		// Convert raw value to bool.
		bool boolPinValue;
		VariableFromRaw<bool>(size, data, boolPinValue);

		// Store it.
		inValues[inputIdx] = boolPinValue;

		if (inputIdx == pinChoice)
		{
			pinValue = inValues[pinChoice];
		}
	}

	// Change on selection pin.
	if (pinId == pinChoiceId)
	{
		if (pinChoice >= 0 && pinChoice < inValues.size())
		{
			pinValue = inValues[pinChoice];
		}
	}

	return r;
}*/

/*
int32_t SwitchBoolGui::initialize()
{
	// initialise auto-duplicate pins.
	IMpPinIterator* it;
	if (MP_OK == getHost()->createPinIterator(reinterpret_cast<void**>(&it)))
	{
		it->getCount(inputPinCount_);
		inputPinCount_ -= nonRepeatingPinCount;

		it->release();
	}

	auto r = SeGuiInvisibleBase::initialize();

//	onSetValue();

	return r;
}*/
