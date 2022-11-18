#define _USE_MATH_DEFINES
#include <math.h>
#include ".\SincLowpassFilter.h"

REGISTER_PLUGIN ( SincLowpassFilter, L"SE Sinc Lowpass Filter" );

SincLowpassFilter::SincLowpassFilter( IMpUnknown* host ) : MpBase( host )
	,histIdx(0)
{
	// Register pins.
	initializePin( 0, pinSignal );
	initializePin( 1, pinTaps );
	initializePin( 2, pinOutput );
	initializePin( 3, pinOutput2 );
}


double resamp( double x, float * indat, int alim, double fmax, double fsr, int wnwdth)
{
	double r_g,r_w,r_a,r_snc,r_y; // some local variables
	r_g = 2 * fmax / fsr;            // Calc gain correction factor
	r_y = 0;
	for( int i = -wnwdth/2 ; i < (wnwdth/2)-1 ; ++ i) // For 1 window width
	{
		int j = (int) (x + i);           // Calc input sample index

		// calculate von Hann Window. Scale and calculate Sinc
		r_w     = 0.5 - 0.5 * cos(2.0 * M_PI * (0.5 + (j - x)/wnwdth) );
		r_a     = 2.0 * M_PI * (j - x) * fmax/fsr;
		r_snc   = 1;

		if( r_a != 0.0 )
		{
			r_snc = sin(r_a) / r_a;
		}

		if( (j >= 0) && (j < alim) )
		{
			r_y = r_y + r_g * r_w * r_snc * indat[j];
		}
	}
	return r_y;                   // Return new filtered sample
}

void SincLowpassFilter::subProcess( int bufferOffset, int sampleFrames )
{
	// get pointers to in/output buffers.
	float* signal	= bufferOffset + pinSignal.getBuffer();
	float* output	= bufferOffset + pinOutput.getBuffer();
//	float* output2	= bufferOffset + pinOutput2.getBuffer();

#if 1
	// write new values.
	memcpy( &(hist[numCoefs]), signal, sizeof(float) * sampleFrames );

	float* h = hist;

#if 0
	for( int s = sampleFrames; s > 0; --s )
	{
		*output++ = filter_.ProcessFiSingle( h++, coefs_ );
	}
#else
	/*
	for( int s = sampleFrames; s > 0; --s )
	{
		float sum = 0;
		for( int t = 0; t < numCoefs; ++t )
		{
			sum += h[t] * taps[t];
		}
		*output++ = sum;
		++h;
	}
	*/

	memset( output, 0, sizeof(float) * sampleFrames );

	h = hist;
	int unalignedPreCount = ( -(intptr_t) output >> 2 ) & 0x3;
	for( int t = 0; t < numCoefs; ++t )
	{
		for( int s = 0; s < sampleFrames; ++s )
		{
			output[s] += h[s] * taps[t];
		}

		/* SSE slower
		// process fiddly non-sse-aligned prequel.
		int s = 0;
		float* o = output;
		for( ; s < unalignedPreCount; ++s )
		{
			*o++ += h[s] * taps[t];
		}

		__m128* out = ( __m128* ) o;
		const __m128 tap = _mm_set_ps1( taps[t] );
		for( ; s < sampleFrames + 3; s += 4 )
		{
			__m128 h4 = _mm_loadu_ps( h + s );
			*out++ = _mm_add_ps( *out, _mm_mul_ps( h4, tap ) );
		}
		*/

		++h;
	}

#endif
	// shift history.
	memcpy( hist, &hist[sampleFrames], sizeof(float) * numCoefs );

#else
	for( int s = sampleFrames; s > 0; --s )
	{
		hist[histIdx++] = *signal++;
		if( histIdx == numCoefs )
		{
			histIdx = 0;
		}

		// convolution.
		float sum = 0;
		int x = histIdx;
		for( int t = 0; t < numCoefs; ++t )
		{
			sum += hist[x++] * taps[t];
			if( x == numCoefs )
			{
				x = 0;
			}
		}
		*output++ = sum;
	}
#endif

//		*output++ = taps[histIdx];
/*
		float buffer[1000];
		x = histIdx;
		for( int i = 0 ; i < tapCount ; ++i )
		{
			buffer[i] = hist[x++];
			if( x == maxTaps )
			{
				x = 0;
			}
		}
		*output2++ = (float) resamp( tapCount/2, buffer, tapCount, 0.25, 1.0, 171 ); // tapCount );
		*/
}

void SincLowpassFilter::onSetPins(void)
{
	// Check which pins are updated.

	if( pinTaps.isUpdated() )
	{
		// reduce cuttoff to account for slope.
		double cuttoff = 0.5 * (22050.0 /*- getSampleRate() * 0.1 */) / getSampleRate();
		calcWindowedSinc( cuttoff, pinTaps, taps );
	}


	// Set state of output audio pins.
	pinOutput.setStreaming(true);

	// Set processing method.
	SET_PROCESS(&SincLowpassFilter::subProcess);

	// Set sleep mode (optional).
	// setSleep(false);
}

int32_t SincLowpassFilter::open()
{
	MpBase::open();	// always call the base class.
/*
	int32_t handle;
	getHost()->getHandle(handle);

	wchar_t name[40];
	swprintf(name, sizeof(name)/sizeof(wchar_t), L"SE SincLowpassFilter %x", handle);

	int32_t need_initialise;
	getHost()->allocateSharedMemory(name, (void**) &taps, -1, maxTaps * sizeof(float), need_initialise);

	if( need_initialise != 0 )
	{
		calcWindowedSinc( 1.0/8.0, size, taps );
	}
	*/
	int32_t blockSize;
	getHost( )->getBlockSize( blockSize );
	int histSize = numCoefs + blockSize;
	hist = new float[histSize];
	memset( hist, 0, sizeof(float) * histSize );

	filter_.Init( numCoefs );

	const int oversampleFactor_ = 2;
	wchar_t name[40];
	swprintf( name, 40, L"Oversampler-SINC %d taps x%d", numCoefs, oversampleFactor_ );

	int32_t needInitialize;
	getHost( )->allocateSharedMemory( name, (void**) &( coefs_.coefs_ ), -1, numCoefs * sizeof( float ), needInitialize );

	// Fill lookup tables if not done already
	coefs_.numCoefs_ = numCoefs;
	if( needInitialize )
	{
		coefs_.InitCoefs( oversampleFactor_ );
	}

	return gmpi::MP_OK;
}
/*




int _tmain(int argc, _TCHAR* argv[])
{
    const int sincTapCount = 171;

    float sincTaps[sincTapCount];

    calcWindowedSinc( 0.25, sincTapCount, sincTaps );
/*
    for( int i = 0 ; i < sincTapCount ; ++i )
    {
        _RPT1( _CRT_WARN, "%f\n", sincTaps[i] );
    }
* /
    int bufferIndex = 0; // whatever
    float samples[2000];
    for( int i = 0 ; i < 2000 ; ++i )
    {
        samples[i] = ( i & 0x20 ) == 0 ? 1.0f : -1.0f;
    }

    float buffer[sincTapCount];
    memset( buffer, 0 , sizeof(buffer) );
    int sampleFrames = 200;


    float* in = samples;

    for( int s = 0 ; s < sampleFrames ; ++s )
    {
        // store input history to bufffer.
        buffer[bufferIndex++] = *in++;
        if( bufferIndex == sincTapCount )
        {
            bufferIndex = 0;
        }

        // convolve SINC FIR with bufer.
        float a = 0;
        int bi = bufferIndex;
        for( int i = 0 ; i < sincTapCount ; ++i )
        {
            a += sincTaps[i] * buffer[bufferIndex++];
            if( bufferIndex == sincTapCount )
            {
                bufferIndex = 0;
            }
        }

        // *out++ - a;
        _RPT2( _CRT_WARN, "%d, %f\n", s, a );
    }

	return 0;
}

*/
/*
void calcWindowedSinc( double cutoff, int sincTapCount, float* returnSincTaps )
{
double r_g,r_w,r_a,r_snc,r_y; // some local variables
r_g = 2 * cutoff;            // Calc gain correction factor
r_y = 0;
for( int k = 0 ; k < sincTapCount ; ++k) // For 1 window width
{
int i = sincTapCount/2 - k;           // Calc input sample index

// calculate von Hann Window. Scale and calculate Sinc
r_w     = 0.5 - 0.5 * cos(2.0 * M_PI * (0.5 + i/ (double)sincTapCount) );
r_a     = 2.0 * M_PI * i * cutoff;
r_snc   = 1;

if( r_a != 0.0 )
{
r_snc = sin(r_a) / r_a;
}

returnSincTaps[k] = (float) (r_g * r_w * r_snc);
}
}
*/