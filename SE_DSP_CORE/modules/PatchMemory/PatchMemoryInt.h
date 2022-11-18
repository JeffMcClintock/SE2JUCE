// Copyright 2007 Jeff McClintock

#ifndef PatchMemoryInt_H_INCLUDED
#define PatchMemoryInt_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"

class PatchMemoryInt: public MpBase
{
public:
	PatchMemoryInt( IMpUnknown* host );
	void onSetPins( void );

private:
	IntInPin pinValueIn;
	IntOutPin pinValueOut;
};

#endif