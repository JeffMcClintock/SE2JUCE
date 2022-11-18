// Copyright 2007 Jeff McClintock

#ifndef PatchMemoryFloat_H_INCLUDED
#define PatchMemoryFloat_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class PatchMemoryFloat: public MpBase
{
public:
	PatchMemoryFloat( IMpUnknown* host );
	void onSetPins( void );

private:
	FloatInPin pinValueIn;
	FloatOutPin pinValueOut;
};

#endif