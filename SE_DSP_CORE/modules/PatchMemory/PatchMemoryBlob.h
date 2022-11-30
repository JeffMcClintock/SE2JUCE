// Copyright 2010 Jeff McClintock

#ifndef PatchMemoryBlob_H_INCLUDED
#define PatchMemoryBlob_H_INCLUDED

#include "mp_sdk_audio.h"

class PatchMemoryBlob: public MpBase
{
	public:
	PatchMemoryBlob(IMpUnknown* host);

	void onSetPins() override;
private:
	BlobInPin pinValueIn;
	BlobOutPin pinValueOut;
};

#endif