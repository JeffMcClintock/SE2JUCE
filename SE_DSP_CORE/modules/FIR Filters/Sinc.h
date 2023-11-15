#pragma once
#ifndef SINC_FILTER_H_INCLUDED
#define SINC_FILTER_H_INCLUDED

/*
#include "Sinc.h"
*/

#include <string.h>
#include <assert.h>
#include "../shared/xp_simd.h"

#define _USE_MATH_DEFINES
#include <math.h>

#if defined( _DEBUG )
#include <iomanip>
#endif

inline void calcWindowedSinc( double cutoff, bool isHighPass, int sincTapCount, float* returnSincTaps )
{
	double r_g,r_w,r_a,r_snc;	// some local variables
	r_g = 2.f * cutoff;         // Calc gain correction factor
	const int centerTap = sincTapCount / 2;
	const float minimumkHz = 0.0000001f;

	if (cutoff < minimumkHz)
	{
		for (int k = 0; k < sincTapCount; ++k)
			returnSincTaps[k] = 0.0f;

		if (isHighPass)
			returnSincTaps[centerTap] = -1.0f;

		return;
	}

	double filterInvertConst = 2.0 * M_PI * cutoff;
	if (isHighPass)
		filterInvertConst = -filterInvertConst;
#if 1
	for (int k = 0; k < sincTapCount; ++k) // For 1 window width
	{
		int i = sincTapCount / 2 - k;       // Calc input sample index

		// calculate von Hann Window. Scale and calculate Sinc
		r_w = 0.5 - 0.5 * cos(2.0 * M_PI * (0.5 + i / (double)sincTapCount));	// Window
		r_a = filterInvertConst * i;											// Filter
		r_snc = sin(r_a) / r_a;

		returnSincTaps[k] = (float)(r_g * r_w * r_snc);
	}
#else
	for (int i = 1; i < sincTapCount / 2; ++i) // For 1 window width
	{
		// calculate von Hann Window. Scale and calculate Sinc
		r_w = 0.5 - 0.5 * cos(2.0 * M_PI * (0.5 + i / (double)sincTapCount));	// Window
		r_a = filterInvertConst * i;											// Filter
		r_snc = sin(r_a) / r_a;

		returnSincTaps[centerTap + i] = returnSincTaps[centerTap - i] = (float)(r_g * r_w * r_snc);
	}
#endif
	if (isHighPass)
		returnSincTaps[centerTap] = static_cast<float>(-1.0 + r_g); // correct divide-by-zero in loop.
	else
		returnSincTaps[centerTap] = (float)r_g; // correct divide-by-zero in loop.
}

struct SincFilterCoefs
{
	int numCoefs_;
	float* coefs_;

	inline void InitCoefs( int oversampleFactor_ )
	{
		calcWindowedSinc( 0.5 / oversampleFactor_, false, numCoefs_, coefs_ );
	}
};

class SincFilter
{
	float* hist;
	int histIdx;
	static const int sseCount = 4; // allocate a few entries off end for SSE.

public:
    SincFilter() : histIdx(0), hist(0)
    {
    }

	~SincFilter()
	{
		delete [] hist;
	}

	// Down-sampling. Put sample in history buffer, but don't calulate output.
	inline void ProcessHistory(float in, int numCoefs )
	{
		// duplicate first 3 samples 'off-end' to ease SSE.
		if( histIdx < sseCount )
		{
			hist[numCoefs + histIdx] = in;
		}

		hist[histIdx++] = in;

		if( histIdx == numCoefs )
		{
			histIdx = 0;
		}
	}

	// Process single sample returning filter output.
	inline float ProcessFiSingle( float* history, const SincFilterCoefs& coefs )
	{
#ifndef GMPI_SSE_AVAILABLE
		// convolution.
		float sum = 0;
		int x = 0;
		for( int t = 0; t < coefs.numCoefs_; ++t )
		{
			sum += hist[x++] * coefs.coefs_[t];
		}
		return sum;
#else
		int numCoefs = coefs.numCoefs_;
		assert( ( numCoefs & 0x03 ) == 0 ); // factor of 4?

		// SSE intrinsics
		__m128 sum1 = _mm_setzero_ps( );
		__m128* pIn2 = ( __m128* ) coefs.coefs_;

		const float* pIn1 = history;

		int sseCount1 = numCoefs >> 2; // remaining samples to end of hist buffer.

		for( int i = sseCount1; i > 0; --i )
		{
			// History samples are unaligned, hence _mm_loadu_ps().
			sum1 = _mm_add_ps( _mm_mul_ps( _mm_loadu_ps( pIn1 ), *pIn2++ ), sum1 );
			pIn1 += 4;
		}

		// horizontal add
// wrong		return sum1[0] + sum1[1] + sum1[2] + sum1[3];
		// more correct to use _mm_store_ss. (not meant to access contents of __m128 directly, might be in register).
		__m128 t = _mm_add_ps(sum1, _mm_movehl_ps(sum1, sum1));
		auto sum2 = _mm_add_ss(t, _mm_shuffle_ps(t, t, 1));
		float sum;
		_mm_store_ss(&sum, sum2);
		return sum;
#endif
	}

	// Process single sample returning filter output.
	inline float ProcessIISingle( float in, const SincFilterCoefs& coefs )
	{
		ProcessHistory( in, coefs.numCoefs_ );

#ifndef GMPI_SSE_AVAILABLE
		// convolution.
		float sum = 0;
		int x = histIdx;
		for( int t = 0 ; t < coefs.numCoefs_ ; ++t )
		{
			sum += hist[x++] * coefs.coefs_[t];
			if( x == coefs.numCoefs_ )
			{
				x = 0;
			}
		}
		return sum;
#else
		int numCoefs = coefs.numCoefs_;
		assert( ( numCoefs & 0x03 ) == 0 ); // factor of 4?

		// SSE intrinsics
		__m128 sum1 = _mm_setzero_ps( );
		__m128* pIn2 = ( __m128* ) coefs.coefs_;

		const float* pIn1 = &( hist[histIdx] );
		const float* histEnd = &( hist[numCoefs] );
#if 1 // non-unrolled
		int sseCount1 = ( 3 + numCoefs - histIdx ) >> 2; // remaining samples to end of hist buffer.

		for( int i = sseCount1; i > 0; --i )
		{
			// History samples are unaligned, hence _mm_loadu_ps().
			sum1 = _mm_add_ps( _mm_mul_ps( _mm_loadu_ps( pIn1 ), *pIn2++ ), sum1 );
			pIn1 += 4;
		}

//		pIn1 -= ( numCoefs >> 2 );
		pIn1 -= ( numCoefs );
		int sseCount2 = ( numCoefs >> 2 ) - sseCount1; // samples fromstart of hist buffer.
		for( int i = sseCount2; i > 0; --i )
		{
			sum1 = _mm_add_ps( _mm_mul_ps( _mm_loadu_ps( pIn1 ), *pIn2++ ), sum1 );
			pIn1 += 4;
		}

#else
		//slower
		__m128 sum2 = _mm_setzero_ps( );
		for( int i = numCoefs >> 3; i > 0; --i )
		{
			// History samples are unaligned, hence _mm_loadu_ps().
			sum1 = _mm_add_ps( _mm_mul_ps( _mm_loadu_ps( pIn1 ), *pIn2++ ), sum1 );
			pIn1 += 4;
			if( pIn1 > histEnd )
			{
				pIn1 -= numCoefs;
			}

			sum2 = _mm_add_ps( _mm_mul_ps( _mm_loadu_ps( pIn1 ), *pIn2++ ), sum2 );
			pIn1 += 4;
			if( pIn1 > histEnd )
			{
				pIn1 -= numCoefs;
			}
		}
		sum1 = _mm_add_ps( sum2, sum1 );
#endif
		// horizontal add
// wrong		return sum1[0] + sum1[1] + sum1[2] + sum1[3];
		// more correct to use _mm_store_ss. (not meant to access contents of __m128 directly, might be in register).
		__m128 t = _mm_add_ps(sum1, _mm_movehl_ps(sum1, sum1));
		auto sum2 = _mm_add_ss(t, _mm_shuffle_ps(t, t, 1));
		float sum;
		_mm_store_ss(&sum, sum2);

		return sum;

		/* slower
		// horizontal add.
		const __m128 t = _mm_add_ps( sum, _mm_movehl_ps( sum, sum ) );
		const __m128 sumx = _mm_add_ss( t, _mm_shuffle_ps( t, t, 1 ) );

		return sumx.m128_f32[0];
		*/
#endif
	}

	// Process single sample returning filter output.
	inline float ProcessDownsample( const SincFilterCoefs& coefs, int convPhase, int oversampleFactor )
	{
		// convolution.
		float sum = 0;
		int x = histIdx - coefs.numCoefs_ / oversampleFactor;
		if( x < 0 )
		{
			x += coefs.numCoefs_;
		}
		for (int t = convPhase; t < coefs.numCoefs_; t += oversampleFactor)
		{
//			_RPT3(_CRT_WARN, "%f * %f (coefs_[%d])\n", hist[x], coefs.coefs_[t], t);
			sum += hist[x++] * coefs.coefs_[t];
			if (x == coefs.numCoefs_)
			{
				x = 0;
			}
		}
		return sum;
	}


	void Init( int numCoefs )
	{
		int size = numCoefs + sseCount;
		hist = new float[size];
		memset( hist, 0, sizeof(float) * size );
	}
};

#endif

