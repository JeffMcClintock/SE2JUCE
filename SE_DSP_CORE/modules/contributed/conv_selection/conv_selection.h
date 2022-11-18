//Convolution selection - by Xhun Audio

#ifndef conv_selection_H_INCLUDED
#define conv_selection_H_INCLUDED

#include "mp_sdk_audio.h"

class conv_selection : public MpBase
{
public:
	conv_selection( IMpUnknown* host );
	void subProcess1( int bufferOffset, int sampleFrames );
	void subProcess2( int bufferOffset, int sampleFrames );
	void subProcess3( int bufferOffset, int sampleFrames );
	virtual void onSetPins(void);

private:
	int 	IOTA;
	float 	fVec0[512];

	AudioInPin pinSignalIn;
	AudioOutPin pinSignalOut;
	IntInPin pinCoeffSel;
};

#endif

