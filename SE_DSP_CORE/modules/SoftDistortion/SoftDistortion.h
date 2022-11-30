#ifndef SOFTDISTORTION_H_INCLUDED
#define SOFTDISTORTION_H_INCLUDED
/*---------------------------------------------------------------------------
	Header for SoftDist.sep
	Version: 1.2, July 2, 2005
	© 2003-2005 David E. Haupt. All rights reserved.

	Built with:
	SynthEdit SDK
	© 2002, SynthEdit Ltd, All Rights Reserved
---------------------------------------------------------------------------*/

#include "../se_sdk3/mp_sdk_audio.h"

class SoftDistortion : public MpBase
{
public:
	SoftDistortion( IMpUnknown* host );
	~SoftDistortion();
	virtual int32_t MP_STDCALL open();
	void subProcess( int bufferOffset, int sampleFrames );
	void onSetPins() override;

private:
	AudioInPin pinSignalIn;
	AudioInPin pinShape;
	AudioInPin pinLimit;
	AudioOutPin pinSignalOut;
	float *tanh_lut;
};

#endif

