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
			if (fpclassify(coefs_[i]) == FP_SUBNORMAL)
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
	static constexpr int sseCount = 4; // allocate a few entries off end for SSE.
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

	float ProcessIISingle_pt2(const float* __restrict pSignal, const float* __restrict pCoefs_f, int todo, int histSize) const;

	// Process single sample returning filter output.
	inline float ProcessIISingle(const int oversampleFactor_)
	{
		const int histSize = (int)hist_.size();

		// convolution.
		// Auto-vectorized C++.
		const int numCoefs = coefs->numCoefs_;
		assert((numCoefs & 0x03) == 0); // factor of 4?

		const float* pCoefs_f = coefs->coefs_;
		const float* pSignal = &(hist_[readIndex_]);

		const int todo = (std::min)(numCoefs, histSize - readIndex_) & 0xfffffffc;

		readIndex_ += oversampleFactor_;
		if (readIndex_ >= histSize)
		{
			readIndex_ -= histSize - sseCount;
		}

		return ProcessIISingle_pt2(pSignal, pCoefs_f, todo, histSize);
	}

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

