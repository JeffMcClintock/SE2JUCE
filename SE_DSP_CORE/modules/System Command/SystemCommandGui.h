#ifndef SYSTEMCOMMANDGUI_H_INCLUDED
#define SYSTEMCOMMANDGUI_H_INCLUDED

#include "mp_sdk_gui.h"

class SystemCommandGui : public MpGuiBase
{
public:
	SystemCommandGui( IMpUnknown* host );

	// overrides
	virtual int32_t MP_STDCALL initialize();

private:
 	void onSetTrigger();

	bool previousTrigger;

 	BoolGuiPin trigger;
 	IntGuiPin command;
 	StringGuiPin commandList;
 	StringGuiPin filename;
};

#endif


