#ifndef SUBPATCHFLOAT_H_INCLUDED
#define SUBPATCHFLOAT_H_INCLUDED

#include "mp_sdk_audio.h"

class SubpatchFloat : public MpBase
{
public:
	SubpatchFloat( IMpUnknown* host );
	virtual void onSetPins(void);

private:
	static const int PatchCount_ = 128;

	BlobInPin pinValueIn;
	EnumInPin pinSubPatch;
	FloatOutPin pinValueOut;
};

#endif

