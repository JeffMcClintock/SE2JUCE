#ifndef SPRING2GUI_H_INCLUDED
#define SPRING2GUI_H_INCLUDED

#include "mp_sdk_gui.h"

class Spring2Gui : public MpGuiBase
{
public:
	Spring2Gui(IMpUnknown* host);

private:
	void onChanged();

	bool prevMouseDown; // only for backward compatible version, prevents spurios signal on startup (may be crashing things).

	FloatGuiPin normalisedValue;
	FloatGuiPin resetValue;
	BoolGuiPin mouseDown;
	BoolGuiPin enable;
};

#endif


