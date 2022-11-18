#ifndef REVERB_H_INCLUDED
#define REVERB_H_INCLUDED

#include "../se_sdk3/mp_sdk_audio.h"
#include "tuning.h"

const float INAUDIBLE = 1.0e-8f;

class Reverb : public MpBase
{
public:
	Reverb( IMpUnknown* host );
	virtual void onSetPins();

	virtual int32_t MP_STDCALL open();
	void subProcess( int bufferOffset, int sampleFrames );
	void subProcessStatic( int bufferOffset, int sampleFrames );
	void subProcessStatic2( int bufferOffset, int sampleFrames );

private:
	static float absf( const float f ) 
	{int i=((*(int*)&f)&0x7fffffff);return (*(float*)&i);}

	static float paramclip(const float param, const float a, const float b)
	{
		return 0.5f * ( absf( param - a ) + a + b - absf ( param - b ) );
	}
	void setwet( float value )
	{
		wet = value * scalewet;
		wet1 = wet * ( half_width + 0.5f );
		wet2 = wet * ( 0.5f - half_width );
	}
	bool no_audible_processing()
	{
		return ( audible_processing_accum < INAUDIBLE && audible_processing_accum > -INAUDIBLE );
	}
	void mute();

	int monocopy;
	float combfeedback;
	float half_width;
	float wet,wet1,wet2;
	float dry;
	float combdamp1;
	float combdamp2;
	float gain;
	float lastgate;
	short mode;
	float dmix;
	float blockSizeRecip;
	float audible_processing_accum; 
	float combfstoreL1,combfstoreL2,combfstoreL3,combfstoreL4;
	float combfstoreL5,combfstoreL6,combfstoreL7,combfstoreL8;
	float combfstoreR1,combfstoreR2,combfstoreR3,combfstoreR4;
	float combfstoreR5,combfstoreR6,combfstoreR7,combfstoreR8;
	int combidxL1,combidxL2,combidxL3,combidxL4;
	int combidxL5,combidxL6,combidxL7,combidxL8;
	int combidxR1,combidxR2,combidxR3,combidxR4;
	int combidxR5,combidxR6,combidxR7,combidxR8;
	float allpassfeedback;
	int allpassidxL1,allpassidxL2,allpassidxL3,allpassidxL4;
	int allpassidxR1,allpassidxR2,allpassidxR3,allpassidxR4;
	int static_count;
	int min_open_samples;
	int mBlockSize;
	bool tail_finished;
	bool input_stat;
	bool buffers_clean;
	bool input_changing;
	// Buffers for the combs
	float	bufcombL1[combtuningL1];
	float	bufcombL2[combtuningL2];
	float	bufcombL3[combtuningL3];
	float	bufcombL4[combtuningL4];
	float	bufcombL5[combtuningL5];
	float	bufcombL6[combtuningL6];
	float	bufcombL7[combtuningL7];
	float	bufcombL8[combtuningL8];
	float	bufcombR1[combtuningR1];
	float	bufcombR2[combtuningR2];
	float	bufcombR3[combtuningR3];
	float	bufcombR4[combtuningR4];
	float	bufcombR5[combtuningR5];
	float	bufcombR6[combtuningR6];
	float	bufcombR7[combtuningR7];
	float	bufcombR8[combtuningR8];
	// Buffers for the allpasses
	float	bufallpassL1[allpasstuningL1];
	float	bufallpassL2[allpasstuningL2];
	float	bufallpassL3[allpasstuningL3];
	float	bufallpassL4[allpasstuningL4];
	float	bufallpassR1[allpasstuningR1];
	float	bufallpassR2[allpasstuningR2];
	float	bufallpassR3[allpasstuningR3];
	float	bufallpassR4[allpasstuningR4];

private:
	AudioInPin pinLIn;
	AudioInPin pinRIn;
	AudioOutPin pinLOut;
	AudioOutPin pinROut;
	AudioInPin pinGate;
	AudioInPin pinSize;
	AudioInPin pinWidth;
	AudioInPin pinDamp;
	AudioInPin pinMix;
	IntInPin pinMode;
};

#endif

