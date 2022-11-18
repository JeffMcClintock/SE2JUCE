/*---------------------------------------------------------------------------
	SoftDistortion.cpp for SoftDist.sep
	Version: 1.2, July 2, 2005
	© 2003-2005 David E. Haupt. All rights reserved.

	Built with:
	SynthEdit SDK
	© 2002, SynthEdit Ltd, All Rights Reserved
---------------------------------------------------------------------------*/
#include <math.h>
#include "./SoftDistortion.h"
#include "../shared/xp_simd.h"

const int TBL_SIZE = 1024;
const int TBL_SIZE_M_1 = TBL_SIZE - 1;
const int HALF_TBL_SIZE = TBL_SIZE / 2; // split +/-

// Waves only
SE_DECLARE_INIT_STATIC_FILE(SoftDistortion);

REGISTER_PLUGIN ( SoftDistortion, L"dehaupt DH_SoftDist 1.2J" );

SoftDistortion::SoftDistortion( IMpUnknown* host ) : MpBase( host )
	,tanh_lut(0)
{
	// Register pins.
	initializePin( 0, pinSignalIn );
	initializePin( 1, pinShape );
	initializePin( 2, pinLimit );
	initializePin( 3, pinSignalOut );
}


void SoftDistortion::onSetPins()
{
	bool OutputIsActive = pinSignalIn.isStreaming();

	// Transmit new output state to modules 'downstream'.
	pinSignalOut.setStreaming( OutputIsActive );

	// Choose which function is used to process audio.
	SET_PROCESS(&SoftDistortion::subProcess);
}


SoftDistortion::~SoftDistortion()
{
	if( tanh_lut ) delete[] tanh_lut;
}


int32_t SoftDistortion::open()
{
	MpBase::open();

	// create & load lut
	tanh_lut = new float[TBL_SIZE+1];
	constexpr float incr = 1.f / HALF_TBL_SIZE;
	float nx = -1.f;
	for( int i = 0; i < TBL_SIZE; ++i )
	{
		tanh_lut[i] = tanhf( 10.f * nx ); // build multiplication by 10 into table
		nx += incr;
	}
	tanh_lut[TBL_SIZE] = tanhf( 10.f * nx ); // fill in last row (for i+1 lookup)

	return gmpi::MP_OK;
}

void SoftDistortion::subProcess( int bufferOffset, int sampleFrames )
{
	// get pointers to in/output buffers.
	float* x = bufferOffset + pinSignalIn.getBuffer();
	float* n = bufferOffset + pinShape.getBuffer();
	float* m = bufferOffset + pinLimit.getBuffer();
	float* y = bufferOffset + pinSignalOut.getBuffer();

	for( int s = sampleFrames - 1; s >= 0; --s )
	{
		float fltidx = ( n[s] * x[s] + 1.f ) * HALF_TBL_SIZE;

		int idx = FastRealToIntTruncateTowardZero(fltidx);
		float fract = fltidx - (float) idx;

		if( idx < 0 ) idx = 0;
		if( idx > TBL_SIZE_M_1 ) idx = TBL_SIZE_M_1;

		y[s] = m[s] * ( tanh_lut[idx] + fract * ( tanh_lut[idx+1] - tanh_lut[idx] ) );
	}
}


