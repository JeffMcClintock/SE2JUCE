#ifndef FLOAT_TO_TEXT_GUI_H_INCLUDED
#define FLOAT_TO_TEXT_GUI_H_INCLUDED

#include "../se_sdk3/mp_sdk_gui.h"
#include "./my_type_convert.h"

class FloatToTextGui : public MpGuiBase
{
public:
	FloatToTextGui( IMpUnknown* host );
	void onInputChanged();
	void onOutputChanged();

private:
 	FloatGuiPin inputValue;
 	IntGuiPin decimalPlaces;
 	StringGuiPin outputValue;
	float approximatedOutput_;
};

#endif
