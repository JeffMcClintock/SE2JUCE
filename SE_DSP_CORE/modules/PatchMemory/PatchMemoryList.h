// Copyright 2006 Jeff McClintock

#ifndef PatchMemoryList_H_INCLUDED
#define PatchMemoryList_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class PatchMemoryList: public MpBase
{
public:
	PatchMemoryList(IMpUnknown* host);

	void onSetPins() override;

private:
	IntInPin pinValueIn;
	EnumOutPin pinValueOut;
};

#endif