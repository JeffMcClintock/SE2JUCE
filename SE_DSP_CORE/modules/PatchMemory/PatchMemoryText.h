// Copyright 2006 Jeff McClintock

#ifndef PatchMemoryText_H_INCLUDED
#define PatchMemoryText_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class PatchMemoryText: public MpBase
{
	public:
	PatchMemoryText(IMpUnknown* host);

	void onSetPins();
private:
	StringInPin pinValueIn;
	StringOutPin pinValueOut;
};

#endif