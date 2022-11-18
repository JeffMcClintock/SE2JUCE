#ifndef GUICONVERTERSGUI_H_INCLUDED
#define GUICONVERTERSGUI_H_INCLUDED

#include "mp_sdk_gui.h"
#include "./my_type_convert.h"

template<typename T1, typename T2>
class SimpleGuiConverter : public MpGuiBase
{
	T1 currentValue;
	bool currentValueNull = true;

	MpGuiPin<T1> inputValue;
	MpGuiPin<T2> outputValue;

public:
	SimpleGuiConverter(IMpUnknown* host) : MpGuiBase(host)
	{
		inputValue.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&SimpleGuiConverter::onInputChanged));
		outputValue.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&SimpleGuiConverter::onOutputChanged));
	}

	void onInputChanged()
	{
		// Prevent visual jitter/sticking with floating point outputs.
		if (inputValue == currentValue && !currentValueNull)
			return;

		currentValue = inputValue;
		currentValueNull = false;
		outputValue = myTypeConvert<T1, T2>(inputValue);
	}

	void onOutputChanged()
	{
		currentValue = myTypeConvert<T2, T1>(outputValue);
		currentValueNull = false;
		inputValue = currentValue;
	}
};

#endif


