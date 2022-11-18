#ifndef TRIGGERATORGUI_H_INCLUDED
#define TRIGGERATORGUI_H_INCLUDED

#include "mp_sdk_gui.h"

class TriggeratorGui : public MpGuiBase
{
public:
	TriggeratorGui( IMpUnknown* host );

	// overrides

private:
 	void onSetBoolVal();
 	void onSetFloatVal();
 	BoolGuiPin Reset;
 	BoolGuiPin boolVal;
 	FloatGuiPin floatVal;

	bool lastButtonDown;
};

#endif


