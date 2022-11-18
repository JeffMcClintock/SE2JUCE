#ifndef SINC_FILTER_H_INCLUDED
#define SINC_FILTER_H_INCLUDED

#include <string.h>
#include <assert.h>
#include "modules/shared/xp_simd.h"

/* 
#include "SincFilter.h"
*/

#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#if defined( _DEBUG )
#include <iomanip>
// Fix for <sstream> on Mac (sstream uses undefined int_64t)
#include "./modules/se_sdk3/mp_api.h"
#include <sstream>
#endif
#include <vector>
#include <algorithm>

inline void calcWindowedSinc( double cutoff, int sincTapCount, float* returnSincTaps )
{
	double r_g,r_w,r_a,r_snc; // some local variables
	r_g = 2 * cutoff;         // Calc gain correction factor

	for( int k = 0; k < sincTapCount; ++k ) // For 1 window width
	{
		int i = sincTapCount / 2 - k;       // Calc input sample index

		// calculate von Hann Window. Scale and calculate Sinc
		r_w = 0.5 - 0.5 * cos( 2.0 * M_PI * ( 0.5 + i / (double) sincTapCount ) );
		r_a = 2.0 * M_PI * i * cutoff;

		r_snc = sin(r_a) / r_a;

		returnSincTaps[k] = (float) (r_g * r_w * r_snc);
	}
	returnSincTaps[sincTapCount / 2] = (float) r_g; // fix divide-by-zero error.

#if 0
	_RPT0(_CRT_WARN, "----SINC-FILTER----------\n");
	for (int k = 0; k < sincTapCount; ++k) // For 1 window width
	{
		_RPT2(_CRT_WARN, "%d, %f\n", k, returnSincTaps[k]);
	}
	_RPT0(_CRT_WARN, "-------------------------\n");
#endif
}

struct SincFilterCoefs
{
	int numCoefs_ = {};
	float* coefs_ = {};

	void InitCoefs(double cuttoff)
	{
		// Allow for transition from passband.
		calcWindowedSinc(cuttoff, numCoefs_, coefs_ );
	}

	void InitCoefsGausian(double cutoff)
	{
		double fir_sum = 0.f;
		for( int k = 0; k < numCoefs_; ++k ) // For 1 window width
		{
			int i = numCoefs_ / 2 - k;       // Calc input sample index

			// calculate von Hann Window. Scale and calculate Sinc
			const auto r_w = 0.5 - 0.5 * cos( 2.0 * M_PI * ( 0.5 + i / (double) numCoefs_ ) );

			const auto r_a = 2.0 * M_PI * i * cutoff;
			double gaussian = exp(-(r_a * r_a)) / M_PI;
			const auto windowed = r_w * gaussian;

			coefs_[k] = (float) windowed;
			fir_sum += windowed;
		}

		// Normalize.
		for (int i = 0; i < numCoefs_; i++)
		{
			coefs_[i] /= (float)fir_sum;
			if (is_denormal(coefs_[i]))
				coefs_[i] = 0.f;
		}


	#if 0
		_RPT0(_CRT_WARN, "----GAUSS-FILTER----------\n");
		for (int k = 0; k < numCoefs_; ++k) // For 1 window width
		{
			_RPT2(_CRT_WARN, "%d, %f\n", k, coefs_[k]);
		}
		_RPT0(_CRT_WARN, "-------------------------\n");
	#endif
	}
};

class SincFilter
{
public:
	static const int sseCount = 4; // allocate a few entries off end for SSE.
	int readahead = 4;

	std::vector<float> hist_;
	int writeIndex_;
	int readIndex_;
	const SincFilterCoefs* coefs = {};

    SincFilter() : writeIndex_(0)
    {
    }

	// Down-sampling. Put sample in history buffer, but don't calulate output.
	inline void ProcessHistory(float in)
	{
		if (writeIndex_ < sseCount)
		{
			hist_[hist_.size() - sseCount + writeIndex_] = in;
		}

		hist_[writeIndex_++] = in;

		if(writeIndex_ == hist_.size() - sseCount)
		{
			writeIndex_ = 0;
		}
	}

	inline void Skip(int incount, int outputcount, const int oversampleFactor_)
	{
		const int histSize = (int)hist_.size();

		// update write pos
		writeIndex_ += incount;
		if (writeIndex_ >= histSize)
		{
			writeIndex_ -= histSize - sseCount;
		}

		// Update read pos.
		readIndex_ += outputcount * oversampleFactor_;
		if (readIndex_ >= histSize)
		{
			readIndex_ -= histSize - sseCount;
		}
	}

	inline void pushHistory(const float* from, int count)
    {
		// copy samples (up to end of hist array).
		int toCopy = (std::min)(count, (int) hist_.size() - writeIndex_);
		float* dest = &(hist_[0]) + writeIndex_;
		for( int i = 0 ; i < toCopy; ++i)
		{
			*dest++ = *from++;
		}
		writeIndex_ += toCopy;

		// wrap and copy remainder to start of hist array.
		count -= toCopy;
		if( count > 0 )
		{
			dest = &(hist_[0]);

			float* wrapSamples = &(hist_[hist_.size() - sseCount]);
			for (int i = 0; i < sseCount; ++i)
			{
				*dest++ = *wrapSamples++;
			}

			for (int i = 0; i < count; ++i)
			{
				*dest++ = *from++;
			}

			writeIndex_ = count + sseCount;
		}
    }

	// Process single sample returning filter output.
	inline float ProcessIISingle( const int oversampleFactor_)
	{
		const int histSize = (int)hist_.size();

		// convolution.
#ifndef GMPI_SSE_AVAILABLE
		int x = readIndex_;
		float sum = 0;
		for( int t = 0 ; t < coefs->numCoefs_ ; ++t )
		{
			sum += hist_[x++] * coefs->coefs_[t];
			if( x == histSize)
			{
				x = sseCount;
			}
		}
#else
		const int numCoefs = coefs->numCoefs_;
		assert( ( numCoefs & 0x03 ) == 0 ); // factor of 4?

		// SSE intrinsics
		__m128 sum1 = _mm_setzero_ps( );
		__m128* pCoefs = ( __m128* ) coefs->coefs_;

        const float* pSignal = &( hist_[readIndex_] );

#if 1 // non-unrolled
    	// Operate on as many as we can up to end of buffer (but no more than num coefs)
		int todo = (std::min)(numCoefs, histSize - readIndex_) & 0xfffffffc;

		for( int i = todo; i > 0; i -= 4 )
		{
			// History samples are unaligned, hence _mm_loadu_ps().
			sum1 = _mm_add_ps( _mm_mul_ps( _mm_loadu_ps( pSignal ), *pCoefs++ ), sum1 );
			pSignal += 4;
		}

		// Process any leftover from start of hist buffer. (wrap).
		pSignal -= histSize - sseCount;
		int remain = numCoefs - todo;
		for( ; remain > 0; remain -= 4)
		{
			sum1 = _mm_add_ps( _mm_mul_ps( _mm_loadu_ps( pSignal ), *pCoefs++ ), sum1 );
			pSignal += 4;
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
        // fail on mac		return sum1.m128_f32[0] + sum1.m128_f32[1] + sum1.m128_f32[2] + sum1.m128_f32[3];
		//return ((float*)&sum1)[0] + ((float*)&sum1)[1] + ((float*)&sum1)[2] + ((float*)&sum1)[3];

		// more correct to use _mm_store_ss. (not meant to access contents of __m128 directly, might be in register).
		__m128 t = _mm_add_ps(sum1, _mm_movehl_ps(sum1, sum1));
		auto sum2 = _mm_add_ss(t, _mm_shuffle_ps(t, t, 1));
		float sum;
		_mm_store_ss(&sum, sum2);
#endif

		readIndex_ += oversampleFactor_;
		if (readIndex_ >= histSize)
		{
			readIndex_ -= histSize - sseCount;
		}

		return sum;
	}
	/*
	// Process single sample returning filter output.
	inline float ProcessDownsample( const SincFilterCoefs& coefs, int convPhase, int oversampleFactor )
	{
		// convolution.
		int x = writeIndex_ - coefs.numCoefs_ / oversampleFactor;
		if( x < 0 )
		{
			x += coefs.numCoefs_;
		}
		float sum = 0;

		for (int t = convPhase; t < coefs.numCoefs_; t += oversampleFactor)
		{
			sum += hist_[x++] * coefs.coefs_[t];
			if (x == coefs.numCoefs_)
			{
				x = 0;
			}
		}


		/ * bit faster
		int total = (coefs.numCoefs_ - convPhase) / oversampleFactor;
		int todo = coefs.numCoefs_ - x;
		if(todo > total)
			todo = total;
		//int todo2 = total - todo;


		int t;
		for(t = convPhase; todo > 0; --todo)
		{
			sum += hist_[x++] * coefs.coefs_[t];
			t += oversampleFactor;
		}

		for(x = 0; t < coefs.numCoefs_; t += oversampleFactor)
		{
			sum += hist_[x++] * coefs.coefs_[t];
		}

		* /
		return sum;
	}
	*/

	int Init(int numCoefs, int /*oversampleFactor*/, int maxBufferSize, int preadahead, const SincFilterCoefs* pcoefs)
	{
		coefs = pcoefs;
		readahead = preadahead;
		// Need enough room to copy incoming samples, plus enough to run coefs over previous samples, plus readahead (latency compensation)
		// readahead should compensate for input upsampler latency to align output sample with 'real' input sample.
		const int bufferSize = maxBufferSize + numCoefs + sseCount + readahead; // enough to ensure input does not overwrite tail before it is filtered.

		hist_.assign(bufferSize, 0.0f);

		readIndex_ = sseCount; // start just after wraparround section.

		writeIndex_ = readIndex_ + numCoefs + preadahead;
/*
		writeIndex_ = sseCount; // start 
		const int effectiveBufferSize = bufferSize - sseCount;

//		readIndex_ = static_cast<int>(hist_.size()) - readahead - numCoefs - sseCount;
		readIndex_ = effectiveBufferSize - (readahead + numCoefs);

const int debugSize = (writeIndex_ - readIndex_ + effectiveBufferSize ) % effectiveBufferSize;
*/
		// return the number of samples that need to be fed in to clear buffer.
		return numCoefs + preadahead;
	}
};

#endif

