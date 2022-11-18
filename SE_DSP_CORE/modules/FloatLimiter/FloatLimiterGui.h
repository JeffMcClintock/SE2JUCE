#ifndef FLOATLIMITERGUI_H_INCLUDED
#define FLOATLIMITERGUI_H_INCLUDED

#include "mp_sdk_gui.h"
#include "TimerManager.h"

class FloatLimiterGui : public MpGuiBase, public TimerClient
{
public:
	FloatLimiterGui( IMpUnknown* host );
	virtual bool OnTimer();

private:
 	void onSetValue();
 	FloatGuiPin min;
 	FloatGuiPin max;
 	FloatGuiPin value;
};

#endif


