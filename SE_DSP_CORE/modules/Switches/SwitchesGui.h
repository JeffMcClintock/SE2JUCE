#ifndef SWITCHBOOLGUI_H_INCLUDED
#define SWITCHBOOLGUI_H_INCLUDED

#include <vector>
#include "../se_sdk3/mp_sdk_gui2.h"

template<typename T>
class GuiSwitch : public SeGuiInvisibleBase
{
	static const int nonRepeatingPinCount = 2;
	static const int pinChoiceId = 0;

public:
	GuiSwitch()
	{
		initializePin(pinChoice);
		initializePin(pinValue, static_cast<MpGuiBaseMemberPtr2>(&GuiSwitch::onSetOutput));
	}

	virtual int32_t MP_STDCALL setPin(int32_t pinId, int32_t voice, int32_t size, const void* data) override
	{
		int32_t r = SeGuiInvisibleBase::setPin(pinId, voice, size, data);

		// Change on one Input pin.
		int inputIdx = pinId - nonRepeatingPinCount;
		if (inputIdx >= 0)
		{
			// In case we havn't counted pins yet.
			// May need to grow input value array to hold this value.
			T defaultvalue = {};

			while (inputIdx >= inValues.size())
			{
				inValues.push_back(defaultvalue);
			}

			// Convert raw value to bool.
			T value;
			VariableFromRaw<T>(size, data, value);

			// Store it.
			inValues[inputIdx] = value;

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
	}

private:
	void onSetOutput()
	{
		if (pinChoice >= 0 && pinChoice < inValues.size())
		{
			auto value = pinValue.getValue();
			int outputPinId = pinChoice + nonRepeatingPinCount;
			inValues[pinChoice] = value;

			getHost()->pinTransmit(outputPinId, variableRawSize(value), variableRawData(value));
		}
	}

	IntGuiPin pinChoice;
	MpGuiPin<T> pinValue;

	std::vector<T> inValues;
};

#endif


