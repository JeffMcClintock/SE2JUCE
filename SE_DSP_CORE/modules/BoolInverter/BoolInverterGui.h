#pragma once
#ifndef BOOLINVERTERGUI_H_INCLUDED
#define BOOLINVERTERGUI_H_INCLUDED

#include "mp_sdk_gui.h"

class BoolInverterGui : public MpGuiBase
{
public:
	BoolInverterGui( IMpUnknown* host );
	virtual int32_t MP_STDCALL initialize() override;

private:
 	void onSetInput();
 	void onSetOutput();
 	BoolGuiPin input;
 	BoolGuiPin output;
};

#endif


