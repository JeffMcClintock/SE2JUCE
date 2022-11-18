// Copyright 2007 Jeff McClintock

#ifndef PatchMemoryBool_H_INCLUDED
#define PatchMemoryBool_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class PatchMemoryBool: public MpBase
{
public:
	PatchMemoryBool( IMpUnknown* host );
	void onSetPins( void );

private:
	BoolInPin pinValueIn;
	BoolOutPin pinValueOut;
};

#endif