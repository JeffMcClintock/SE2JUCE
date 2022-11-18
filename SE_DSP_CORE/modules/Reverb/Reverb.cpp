/*---------------------------------------------------------------------------
	Reverb.cpp for DH_Reverb.sem
	Version: 1.0  March 14, 2006
	� 2006 David E. Haupt. All rights reserved.

	Built with:
	SynthEdit SDK
	� 2002, SynthEdit Ltd, All Rights Reserved
---------------------------------------------------------------------------*/
#define _CRT_SECURE_NO_DEPRECATE

#include <math.h>
#include <stdio.h>
#include "./Reverb.h"

SE_DECLARE_INIT_STATIC_FILE(Reverb);

REGISTER_PLUGIN ( Reverb, L"DH_Reverb 1.0J" );

#define ZeroOutMemory(addr,size) memset(addr,0,size)
#define kill_denormal(sample) if(((*(unsigned int*)&sample)&0x7f800000)==0) sample=0.0f

enum 
{ 
	MODE_NORMAL,
	MODE_FREEZE,
	MODE_GATED,
};
						 
enum 
{ 
	NO, 
	YES 
};


Reverb::Reverb( IMpUnknown* host ) : MpBase( host )
,blockSizeRecip(0.f)
,mode(MODE_NORMAL)
,lastgate(0.f)
,dmix(0.f)
,monocopy(0)
,static_count(0)
,min_open_samples(0)
,mBlockSize(0)
,tail_finished(false)
,input_stat(false)
,buffers_clean(true)
,input_changing(false)

,combfeedback(0.f)
,combfstoreL1(0.f),combfstoreL2(0.f),combfstoreL3(0.f),combfstoreL4(0.f)
,combfstoreL5(0.f),combfstoreL6(0.f),combfstoreL7(0.f),combfstoreL8(0.f)
,combfstoreR1(0.f),combfstoreR2(0.f),combfstoreR3(0.f),combfstoreR4(0.f)
,combfstoreR5(0.f),combfstoreR6(0.f),combfstoreR7(0.f),combfstoreR8(0.f)
,combdamp1(0.f),combdamp2(0.f)
,combidxL1(0),combidxL2(0),combidxL3(0),combidxL4(0)
,combidxL5(0),combidxL6(0),combidxL7(0),combidxL8(0)
,combidxR1(0),combidxR2(0),combidxR3(0),combidxR4(0)
,combidxR5(0),combidxR6(0),combidxR7(0),combidxR8(0)
,allpassfeedback(0.5f)
,allpassidxL1(0),allpassidxL2(0),allpassidxL3(0),allpassidxL4(0)
,allpassidxR1(0),allpassidxR2(0),allpassidxR3(0),allpassidxR4(0)
{
	// Register pins.
	initializePin( 0, pinLIn );
	initializePin( 1, pinRIn );
	initializePin( 2, pinLOut );
	initializePin( 3, pinROut );
	initializePin( 4, pinGate );
	initializePin( 5, pinSize );
	initializePin( 6, pinWidth );
	initializePin( 7, pinDamp );
	initializePin( 8, pinMix );
	initializePin( 9, pinMode );
}

int32_t Reverb::open()
{
	MpBase::open();

	half_width = initialwidth * 0.5f;
	setwet(initialwet);
	combfeedback = initialroom * scaleroom + offsetroom;
	dry = initialdry * scaledry;
	combdamp1 = initialdamp * scaledamp;
	combdamp2 = 1.f - combdamp1;		
	mute();
	blockSizeRecip = 1.f / ( mBlockSize = getBlockSize() );
	float sampleRte = getSampleRate();
	min_open_samples = (int)sampleRte / 2; // keep reverb running for at least 500 ms after input stops changing

	SET_PROCESS(&Reverb::subProcess);

	return gmpi::MP_OK;
}

void Reverb::onSetPins()
{
	// resignal to downstream
	bool out_stat = ( tail_finished || buffers_clean ) ? false : true;
	input_stat = pinLIn.isStreaming();

	if( input_stat == true ) // ST_RUN )
	{
		out_stat = true;
	}

	// Set state of output audio pins.
	pinLOut.setStreaming(out_stat);
	pinROut.setStreaming(out_stat);

/* not supported SDK3
	// case PN_INPUT1:
	if( pinLIn.isUpdated() )
	{
		if ( ! pinLIn.IsConnected())
		{
			mute();
			buffers_clean = true;
		}
	}

	if ( pinRIn.IsConnected() )
*/
		monocopy = 0;
/*	else
		monocopy = 1;
*/
	mode = pinMode.getValue();

	// set up sleep mode if inactive
	if( !out_stat )
	{
		static_count = getBlockSize();
		SET_PROCESS(&Reverb::subProcessStatic);
	}
	else
	{
		SET_PROCESS(&Reverb::subProcess);
	}

	// Set sleep mode (optional).
	setSleep(false);
}

void Reverb::subProcess( int bufferOffset, int sampleFrames )
{
	// get pointers to in/output buffers.
	float *pgate  = bufferOffset + pinGate.getBuffer();
	float *psize  = bufferOffset + pinSize.getBuffer();
	float *pwidth = bufferOffset + pinWidth.getBuffer();
	float *pdamp  = bufferOffset + pinDamp.getBuffer();
	float *pmix   = bufferOffset + pinMix.getBuffer();

	combfeedback = paramclip( *psize, 0.01f, 0.98f ) * scaleroom + offsetroom;
	float mix = paramclip( *pmix, -0.5f, 0.5f );
	half_width = paramclip( *pwidth, 0.01f, 0.98f ) * 0.5f;
	float wet0 = mix + 0.5f;
	float dry0 = 0.5f - mix;
	float prod = wet0 * dry0;
	setwet( wet0 + prod );
	dry = ( dry0 + prod ) * scaledry;
	combdamp1 = paramclip( *pdamp, 0.01f, 1.f ) * scaledamp;
	combdamp2 = 1.f - combdamp1;		
	gain = fixedgain;

	switch( mode )
	{
	case MODE_FREEZE:			//FREEZE MODE
		if ( *pgate > 0.f )					// if gate UP (evaluated only once per block)
		{
			combfeedback = 1.f;		// freeze
			combdamp1 = 0.f;
			combdamp2 = 1.f;
			gain = muted;
		}
		break;
	case MODE_GATED:			//GATED MODE
		if ( *pgate > 0.f )					// if gate UP (evaluated only once per block)
		{
			lastgate = *pgate;
		}
		else
		{
			if ( lastgate > 0.0f )			// if closing the gate
			{								// crossmix with the input
				setwet( ( 1.f - dmix ) * ( mix + 0.5f ) );
				if ( dmix < 1.f )
				{
					dmix += blockSizeRecip; // fade over 1 block
				}
				else
				{
					lastgate = 0.f;    // done fade
				}
			}
			else						// else gate closed -- no reverb
			{
				dmix = 0.f;
				setwet( 0.f );
			}
		}
		break;
	default:
		;  // nothing to be done for Normal Mode 
	}
	
	float *inputL = bufferOffset + pinLIn.getBuffer();
	float *inputR = bufferOffset + pinRIn.getBuffer();
	if ( monocopy )	inputR = inputL;
	float *outputL = bufferOffset + pinLOut.getBuffer();
	float *outputR = bufferOffset + pinROut.getBuffer();
	audible_processing_accum = 0.f;
	int s = sampleFrames;
	while( s-- > 0)
	{
		float input;
		audible_processing_accum += input = ( *inputL + *inputR ) * gain;

		float outL ,outR, out1, out2, out3, out4, out5, out6, out7, out8;

		combfstoreL1 = ( out1 = bufcombL1[combidxL1] ) * combdamp2 + combfstoreL1 * combdamp1;
		combfstoreL2 = ( out2 = bufcombL2[combidxL2] ) * combdamp2 + combfstoreL2 * combdamp1;
		combfstoreL3 = ( out3 = bufcombL3[combidxL3] ) * combdamp2 + combfstoreL3 * combdamp1;
		combfstoreL4 = ( out4 = bufcombL4[combidxL4] ) * combdamp2 + combfstoreL4 * combdamp1;
		combfstoreL5 = ( out5 = bufcombL5[combidxL5] ) * combdamp2 + combfstoreL5 * combdamp1;
		combfstoreL6 = ( out6 = bufcombL6[combidxL6] ) * combdamp2 + combfstoreL6 * combdamp1;
		combfstoreL7 = ( out7 = bufcombL7[combidxL7] ) * combdamp2 + combfstoreL7 * combdamp1;
		combfstoreL8 = ( out8 = bufcombL8[combidxL8] ) * combdamp2 + combfstoreL8 * combdamp1;
		outL = out1 + out2 + out3 + out4 + out5 + out6 + out7 + out8;

		bufcombL1[combidxL1] = input + combfstoreL1 * combfeedback;
		bufcombL2[combidxL2] = input + combfstoreL2 * combfeedback;
		bufcombL3[combidxL3] = input + combfstoreL3 * combfeedback;
		bufcombL4[combidxL4] = input + combfstoreL4 * combfeedback;
		bufcombL5[combidxL5] = input + combfstoreL5 * combfeedback;
		bufcombL6[combidxL6] = input + combfstoreL6 * combfeedback;
		bufcombL7[combidxL7] = input + combfstoreL7 * combfeedback;
		bufcombL8[combidxL8] = input + combfstoreL8 * combfeedback;

		if( ++combidxL1 >= combtuningL1 ) combidxL1 = 0;
		if( ++combidxL2 >= combtuningL2 ) combidxL2 = 0;
		if( ++combidxL3 >= combtuningL3 ) combidxL3 = 0;
		if( ++combidxL4 >= combtuningL4 ) combidxL4 = 0;
		if( ++combidxL5 >= combtuningL5 ) combidxL5 = 0;
		if( ++combidxL6 >= combtuningL6 ) combidxL6 = 0;	
		if( ++combidxL7 >= combtuningL7 ) combidxL7 = 0;
		if( ++combidxL8 >= combtuningL8 ) combidxL8 = 0;	

		combfstoreR1 = ( out1 = bufcombR1[combidxR1] ) * combdamp2 + combfstoreR1 * combdamp1;
		combfstoreR2 = ( out2 = bufcombR2[combidxR2] ) * combdamp2 + combfstoreR2 * combdamp1;
		combfstoreR3 = ( out3 = bufcombR3[combidxR3] ) * combdamp2 + combfstoreR3 * combdamp1;
		combfstoreR4 = ( out4 = bufcombR4[combidxR4] ) * combdamp2 + combfstoreR4 * combdamp1;
		combfstoreR5 = ( out5 = bufcombR5[combidxR5] ) * combdamp2 + combfstoreR5 * combdamp1;
		combfstoreR6 = ( out6 = bufcombR6[combidxR6] ) * combdamp2 + combfstoreR6 * combdamp1;
		combfstoreR7 = ( out7 = bufcombR7[combidxR7] ) * combdamp2 + combfstoreR7 * combdamp1;
		combfstoreR8 = ( out8 = bufcombR8[combidxR8] ) * combdamp2 + combfstoreR8 * combdamp1;
		outR = out1 + out2 + out3 + out4 + out5 + out6 + out7 + out8;

		bufcombR1[combidxR1] = input + combfstoreR1 * combfeedback;
		bufcombR2[combidxR2] = input + combfstoreR2 * combfeedback;
		bufcombR3[combidxR3] = input + combfstoreR3 * combfeedback;
		bufcombR4[combidxR4] = input + combfstoreR4 * combfeedback;
		bufcombR5[combidxR5] = input + combfstoreR5 * combfeedback;
		bufcombR6[combidxR6] = input + combfstoreR6 * combfeedback;
		bufcombR7[combidxR7] = input + combfstoreR7 * combfeedback;
		bufcombR8[combidxR8] = input + combfstoreR8 * combfeedback;

		if( ++combidxR1 >= combtuningR1 ) combidxR1 = 0;
		if( ++combidxR2 >= combtuningR2 ) combidxR2 = 0;
		if( ++combidxR3 >= combtuningR3 ) combidxR3 = 0;
		if( ++combidxR4 >= combtuningR4 ) combidxR4 = 0;
		if( ++combidxR5 >= combtuningR5 ) combidxR5 = 0;
		if( ++combidxR6 >= combtuningR6 ) combidxR6 = 0;	
		if( ++combidxR7 >= combtuningR7 ) combidxR7 = 0;
		if( ++combidxR8 >= combtuningR8 ) combidxR8 = 0;

		float bufout;

		bufout = bufallpassL1[allpassidxL1];
		out1 = -outL + bufout;
		bufallpassL1[allpassidxL1] = outL + bufout * allpassfeedback;

		bufout = bufallpassL2[allpassidxL2];
		out2 = -out1 + bufout;
		bufallpassL2[allpassidxL2] = out1 + bufout * allpassfeedback;

		bufout = bufallpassL3[allpassidxL3];
		out3 = -out2 + bufout;
		bufallpassL3[allpassidxL3] = out2 + bufout * allpassfeedback;

		bufout = bufallpassL4[allpassidxL4];
		outL = -out3 + bufout;
		bufallpassL4[allpassidxL4] = out3 + bufout * allpassfeedback;

		if( ++allpassidxL1 >= allpasstuningL1 ) allpassidxL1 = 0;
		if( ++allpassidxL2 >= allpasstuningL2 ) allpassidxL2 = 0;
		if( ++allpassidxL3 >= allpasstuningL3 ) allpassidxL3 = 0;
		if( ++allpassidxL4 >= allpasstuningL4 ) allpassidxL4 = 0;

		bufout = bufallpassR1[allpassidxR1];
		out1 = -outR + bufout;
		bufallpassR1[allpassidxR1] = outR + bufout * allpassfeedback;

		bufout = bufallpassR2[allpassidxR2];
		out2 = -out1 + bufout;
		bufallpassR2[allpassidxR2] = out1 + bufout * allpassfeedback;

		bufout = bufallpassR3[allpassidxR3];
		out3 = -out2 + bufout;
		bufallpassR3[allpassidxR3] = out2 + bufout * allpassfeedback;

		bufout = bufallpassR4[allpassidxR4];
		outR = -out3 + bufout;
		bufallpassR4[allpassidxR4] = out3 + bufout * allpassfeedback;

		if( ++allpassidxR1 >= allpasstuningR1 ) allpassidxR1 = 0;
		if( ++allpassidxR2 >= allpasstuningR2 ) allpassidxR2 = 0;
		if( ++allpassidxR3 >= allpasstuningR3 ) allpassidxR3 = 0;
		if( ++allpassidxR4 >= allpasstuningR4 ) allpassidxR4 = 0;

		audible_processing_accum += *outputL = (outL * wet1) + (outR * wet2) + (*inputL * dry);
//		kill_denormal(*outputL);
		audible_processing_accum += *outputR = (outR * wet1) + (outL * wet2) + (*inputR * dry);
//		kill_denormal(*outputR);
		++inputL;
		++inputR;
		++outputL;
		++outputR;
	}

	inputL = bufferOffset + pinLIn.getBuffer();
	inputR = bufferOffset + pinRIn.getBuffer();
	outputL = bufferOffset + pinLOut.getBuffer();
	outputR = bufferOffset + pinROut.getBuffer();
	buffers_clean = false;
	float input_change = *inputL - *(inputL+sampleFrames-1) 
						+ *inputR - *(inputR+sampleFrames-1);

	// test 1st and last samples for changing input
	if( input_change > INAUDIBLE || input_change < -INAUDIBLE )
	{
		input_changing = true;
		tail_finished = false;
		SET_PROCESS(&Reverb::subProcess);
	}
	else if( input_changing ) // input has stopped changing
	{
		input_changing = false;
		static_count = min_open_samples;
	}
	else
	{
		if( ! tail_finished ) static_count -= sampleFrames;

		// test for end of reverb tail & set up for sleep mode if done
		if( static_count <= 0 && no_audible_processing() )
		{
			if( ! tail_finished )
			{
				tail_finished = true;

				// fade output & clear reverb buffers
				float sampleFrames_recip = 1.f / sampleFrames;
				for( int s = sampleFrames - 1; s >= 0; --s )
				{
					float r = s * sampleFrames_recip;				
					*outputL++ *= r;  
					*outputR++ *= r;
				}
				if( ! buffers_clean )
				{
					mute();
					buffers_clean = true;
				}
				static_count = mBlockSize;
				SET_PROCESS(&Reverb::subProcessStatic);
			}
		}
	}
} // subProcess


void Reverb::mute()
{
	if ( mode == MODE_FREEZE ) return;

	ZeroOutMemory(	bufcombL1, sizeof(float) * combtuningL1 );
	ZeroOutMemory(	bufcombL2, sizeof(float) * combtuningL2 );
	ZeroOutMemory(	bufcombL3, sizeof(float) * combtuningL3 );
	ZeroOutMemory(	bufcombL4, sizeof(float) * combtuningL4 );
	ZeroOutMemory(	bufcombL5, sizeof(float) * combtuningL5 );
	ZeroOutMemory(	bufcombL6, sizeof(float) * combtuningL6 );
	ZeroOutMemory(	bufcombL7, sizeof(float) * combtuningL7 );
	ZeroOutMemory(	bufcombL8, sizeof(float) * combtuningL8 );
	ZeroOutMemory(	bufcombR1, sizeof(float) * combtuningR1 );
	ZeroOutMemory(	bufcombR2, sizeof(float) * combtuningR2 );
	ZeroOutMemory(	bufcombR3, sizeof(float) * combtuningR3 );
	ZeroOutMemory(	bufcombR4, sizeof(float) * combtuningR4 );
	ZeroOutMemory(	bufcombR5, sizeof(float) * combtuningR5 );
	ZeroOutMemory(	bufcombR6, sizeof(float) * combtuningR6 );
	ZeroOutMemory(	bufcombR7, sizeof(float) * combtuningR7 );
	ZeroOutMemory(	bufcombR8, sizeof(float) * combtuningR8 );
	ZeroOutMemory(	bufallpassL1, sizeof(float) * allpasstuningL1 );
	ZeroOutMemory(	bufallpassL2, sizeof(float) * allpasstuningL2 );
	ZeroOutMemory(	bufallpassL3, sizeof(float) * allpasstuningL3 );
	ZeroOutMemory(	bufallpassL4, sizeof(float) * allpasstuningL4 );
	ZeroOutMemory(	bufallpassR1, sizeof(float) * allpasstuningR1 );
	ZeroOutMemory(	bufallpassR2, sizeof(float) * allpasstuningR2 );
	ZeroOutMemory(	bufallpassR3, sizeof(float) * allpasstuningR3 );
	ZeroOutMemory(	bufallpassR4, sizeof(float) * allpasstuningR4 );
}



// sub process for sleep mode
void Reverb::subProcessStatic( int bufferOffset, int sampleFrames )
{
	subProcess(bufferOffset, sampleFrames);
	static_count -= sampleFrames;

	if( static_count <= 0 && ( buffers_clean || tail_finished ) )
	{
		if( !input_stat ) //< ST_RUN )
		{
			// CallHost(seaudioMasterSleepMode);
			setSleep(true);
		}
		else
		{
			SET_PROCESS(&Reverb::subProcessStatic2);
		}
	}
}


// sub process for idle no-sleep mode
void Reverb::subProcessStatic2( int bufferOffset, int sampleFrames )
{
	float *in1 = bufferOffset + pinLIn.getBuffer();
	bool inputResumed = false;
	for( int s = sampleFrames; s > 0; s-- )
	{
		if( *in1++ > INAUDIBLE ) // change this to test for changing input?
		{
			inputResumed = true;
			break;
		}
	}
	if( inputResumed )
	{
		SET_PROCESS(&Reverb::subProcess);
		subProcess(bufferOffset, sampleFrames);
	}
}


