#pragma once
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
//#include <cmath>
#include "ug_base.h"
#include "conversion.h"

class UpsamplingInterpolator // cubic.
{
	float history[17];

public:
	static int calcLatency(int factor)
	{
		return factor * 4;
	}
	static int calcCoefsMemoryBytes(int /*factor*/)
	{
		return 0;
	}
	static void initCoefs(int /*factor*/, float* /*coefs*/)
	{}

	void Init(int /*numCoefs*/, int /*maxBufferSize*/)
	{
		memset(history, 0, sizeof(history));
		history[16] = 1234.0f;
	}

	void process(int outSampleframes, float* from, float* to, int subSample, int oversampleFactor, float* /*coefs_unused*/)
	{
//		_RPT0(_CRT_WARN, "XXX UpsamplingInterpolator\n");

		int inIdx = 0;
		float* buffer = history + 4;

		history[inIdx + 8] = from[inIdx]; // incrementally fill history
		const double inveseOsFactor = 1.0 / (double) oversampleFactor;
		for (int s = outSampleframes; s > 0; --s)
		{
			double fraction = subSample * inveseOsFactor; // / (float)oversampleFactor;

//			*to++ = from[inIdx]; // no interpolation.
//			*to++ = buffer[inIdx] +(buffer[inIdx + 1] - buffer[inIdx]) * fraction; // linear interpolation.

			// Cubic interpolation. Needs to be done at double resolution else tiny errors creep in cause STATIC fails.
			double y0 = buffer[inIdx - 1];
			double y1 = buffer[inIdx + 0];
			double y2 = buffer[inIdx + 1];
			double y3 = buffer[inIdx + 2];

			*to++ = static_cast<float>(y1 + 0.5 * fraction * (y2 - y0 + fraction * (2.0 * y0 - 5.0 * y1 + 4.0 * y2 - y3 + fraction * (3.0 * (y1 - y2) + y3 - y0))));

//			*to++ = static_cast<float>( y1 + fraction * (y2-y1)); // 2-point.
//			*to++ = (float) y1; // 1-point.

//			_RPT4(_CRT_WARN, "XXX buf %f %f %f %f ->", y0, y1, y2, y3);
//			_RPT1(_CRT_WARN, " output %f\n",to[-1]);


//?			assert(to[-1] > -0.65f);

			if (++subSample == oversampleFactor)
			{
				subSample = 0;
				++inIdx;

				// once we have read out small history buffer, use samples directly from input buffer.
				if (inIdx == 8)
				{
					buffer = from - 4;
				}
				else
				{
					if (inIdx < 8 && s > 1) // dosn't count after last sample.
					{
//						assert(from[inIdx] < 20.0f);
						history[inIdx + 8] = from[inIdx]; // incrementally fill history.
//						assert(history[inIdx + 8] < 100.0f);
					}
				}
			}
		}

		// Copy last 12 samples to history.
		const int keepSamples = 8;
		int inSampleFrames = inIdx;
		int shift2 = (std::min)(keepSamples, inSampleFrames);

		for (int i = 0; i < keepSamples - shift2; ++i)
		{
			history[i] = history[shift2 + i];
//			assert(history[i] < 100.0f);
		}

		for (int i = 0; i < shift2; ++i)
		{
			history[keepSamples - shift2 + i] = from[inSampleFrames - shift2 + i];
//			assert(history[keepSamples - shift2 + i] < 100.0f);
		}

		assert(history[16] == 1234.0f);

//		_RPT0(_CRT_WARN, "XXX ----------------------\n");
	}
};
/*
class UpsamplingInterpolator2 // sinc.
{
	static const int histSamples = 16;
	float history[histSamples + 1];

public:
	UpsamplingInterpolator2()
	{
		memset(history, 0, sizeof(history));
		history[histSamples] = 1234.0f;
	}

	static int calcCoefsMemoryBytes(int sincSize, int factor)
	{
		return sizeof(float) * sincSize * factor;
	}

	static void initCoefs(int sincSize, int factor, float* coefs)
	{
		float* dest = coefs;

		// Sub-table one consists of a single "1".
		for (int i = 0; i < sincSize; i++)
		{
			*dest++ = 0.0f;
		}
		coefs[sincSize/2 - 1] = 1.0f;

		//_RPT0(_CRT_WARN, "----------------f\n");
		//for (int i = 0; i < sincSize; i++)
		//{
		//	_RPT1(_CRT_WARN, "%f\n", coefs[i]);
		//}

		// Subsequent tables
		const int tableWidth = sincSize / 2;
		for(int sub_table = 1 ; sub_table < factor ; ++sub_table)
		{
//			_RPT0(_CRT_WARN, "----------------f\n");
			double fir_sum = 0.f;
			for (int i = 0; i < sincSize; i++)
			{
				int i2 = i - tableWidth;

				// position on x axis
				double o = i2 + (factor-sub_table) / (double)factor;
				assert(o != 0.0); // else div by zero.
				double sinc = sin(PI * o) / (PI * o);

				// apply tailing function
				double hanning = cos(0.5 * PI * o / (double)tableWidth);
				float windowed_sinc = (float)(sinc * hanning * hanning);

				*dest++ = windowed_sinc;
				fir_sum += windowed_sinc;
			}

			// Normalize.
			dest -= sincSize;

			for (int i = 0; i < sincSize; i++)
			{
				*dest /= (float) fir_sum;
				if (is_denormal(*dest))
					*dest = 0.f;

//				_RPT1(_CRT_WARN, "%f\n", *dest);
				dest++;
			}
		}
	}

	inline void process(int outSampleframes, float* from, float* to, int subSample, int oversampleFactor, int sincSize, float* coefs)
	{
		int inIdx = 0;
		float* buffer = history + 4;

		history[inIdx + 8] = from[inIdx]; // incrementally fill history
		for (int s = outSampleframes; s > 0; --s)
		{
			// Convolution with sinc.
			float* filter = coefs + sincSize * subSample;
			float sum = 0.0f;
			for(int i = -sincSize/2; i < sincSize / 2; ++ i)
			{
				sum += buffer[inIdx + i] * filter[sincSize / 2 + i];
			}
			*to++ = sum;

			if (++subSample == oversampleFactor)
			{
				subSample = 0;
				++inIdx;

				// once we have read out small history buffer, use samples directly from input buffer.
				if (inIdx == 8)
				{
					buffer = from - 4;
				}
				else
				{
					if (inIdx < 8 && s > 1) // dosn't count after last sample.
					{
						assert(from[inIdx] < 20.0f);
						history[inIdx + 8] = from[inIdx]; // incrementally fill history.
						assert(history[inIdx + 8] < 100.0f);
					}
				}
			}
		}

		// Copy last 12 samples to history.
		const int keepSamples = 8;
		int inSampleFrames = inIdx;
		int shift2 = (std::min)(keepSamples, inSampleFrames);

		for (int i = 0; i < keepSamples - shift2; ++i)
		{
			history[i] = history[shift2 + i];
			assert(history[i] < 100.0f);
		}

		for (int i = 0; i < shift2; ++i)
		{
			history[keepSamples - shift2 + i] = from[inSampleFrames - shift2 + i];
			assert(history[keepSamples - shift2 + i] < 100.0f);
		}

		assert(history[16] == 1234.0f);
	}
};
*/

template <int sincSize>
class UpsamplingInterpolator3 // sinc SSE
{
	static const int sseCount = 4; // allocate a few entries off end for SSE.
	std::vector<float> hist_;
	int writeIndex_ = {};
	int readIndex_ = {};
	const float* coefs = {};

public:
	static int calcLatency(int factor)
	{
		return (sincSize / 2 + 5) * factor; // sincsize/2 is latency due to convolution, 5 is likly SSEsize + 1
	}

	// Need enough room to copy incoming samples, plus enough to run coefs over historic samples, plus readahead (latency compensation)
	int Init(int numCoefs, int maxBufferSize, const float* pcoefs)
	{
		coefs = pcoefs;

		int bufferSize = maxBufferSize + numCoefs + sseCount; // enough to ensure input does not overwrite tail before it is filtered.

		hist_.assign(bufferSize, 0.0f);

		readIndex_ = 0;
		writeIndex_ = numCoefs + sseCount;

		return numCoefs + sseCount; // return through-delay
	}

	static int calcCoefsMemoryBytes(int factor)
	{
		return sizeof(float) * sincSize * factor;
	}

	static void initCoefs(int factor, float* coefs)
	{
		float* dest = coefs;

		// Sub-table one consists of a single "1".
		for (int i = 0; i < sincSize; i++)
		{
			*dest++ = 0.0f;
		}
		coefs[sincSize / 2 - 1] = 1.0f;

		//_RPT0(_CRT_WARN, "----------------f\n");
		//for (int i = 0; i < sincSize; i++)
		//{
		//	_RPT1(_CRT_WARN, "%f\n", coefs[i]);
		//}

		// Subsequent tables
		const int tableWidth = sincSize / 2;
		for (int sub_table = 1; sub_table < factor; ++sub_table)
		{
			//			_RPT0(_CRT_WARN, "----------------f\n");
			double fir_sum = 0.f;
			for (int i = 0; i < sincSize; i++)
			{
				int i2 = i - tableWidth;

				// position on x axis
				double o = i2 + (factor - sub_table) / (double)factor;
				assert(o != 0.0); // else div by zero.
				double sinc = sin(PI * o) / (PI * o);

				// apply tailing function
				double hanning = cos(0.5 * PI * o / (double)tableWidth);
				float windowed_sinc = (float)(sinc * hanning * hanning);

				*dest++ = windowed_sinc;
				fir_sum += windowed_sinc;
			}

			// Normalize.
			dest -= sincSize;

			for (int i = 0; i < sincSize; i++)
			{
				*dest /= (float)fir_sum;
				if (is_denormal(*dest))
					*dest = 0.f;

				//				_RPT1(_CRT_WARN, "%f\n", *dest);
				dest++;
			}
		}
	}

	static void initCoefs_gaussian(int factor, float* coefs)
	{
		float* dest = coefs;

		// Subsequent tables
		const int tableWidth = sincSize / 2;
		for (int sub_table = 0; sub_table < factor; ++sub_table)
		{
			//			_RPT0(_CRT_WARN, "----------------f\n");
			double fir_sum = 0.f;
			for (int i = 0; i < sincSize; i++)
			{
				int i2 = i - tableWidth;

				// position on x axis
				double o = i2 + (factor - sub_table) / (double)factor;
				double gaussian = exp(-(o * o)) / M_PI;

				// apply tailing function
				double hanning = cos(0.5 * PI * o / (double)tableWidth);
				float windowed = (float)(gaussian * hanning * hanning);

				*dest++ = static_cast<float>(windowed);
				fir_sum += windowed;
			}

			// Normalize.
			dest -= sincSize;

			for (int i = 0; i < sincSize; i++)
			{
				*dest /= (float)fir_sum;
				if (is_denormal(*dest))
					*dest = 0.f;

//				_RPT1(_CRT_WARN, "%f, ", *dest);
				dest++;
			}
//			_RPT0(_CRT_WARN, "\n");
		}
	}

	// copy new samples into history buffer, wrapping if nesc.
	void pushHistory(const float* from, int outcount, int subSample, int oversampleFactor)
	{
		auto count = (subSample + outcount) / oversampleFactor;

		// copy samples (up to end of hist array).
		int toCopy = (std::min)(count, (int)hist_.size() - writeIndex_);
		float* dest = &(hist_[0]) + writeIndex_;
		for (int i = 0; i < toCopy; ++i)
		{
			*dest++ = *from++;
		}
		writeIndex_ += toCopy;

		// wrap and copy remainder to start of hist array.
		count -= toCopy;
		if (count > 0)
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
	
	void process(int outSampleframes, float* from, float* to, int subSample, int oversampleFactor)//, float* coefs)
	{
		pushHistory(from, outSampleframes, subSample, oversampleFactor);

		const int histSize = (int)hist_.size();
//		int inIdx = 0;
		for (int s = outSampleframes; s > 0; --s)
		{
			// Convolution with sinc.
			const float* filter = coefs + sincSize * subSample;
#if !GMPI_USE_SSE
			// C++
			float sum = 0.0f;
			int x = readIndex_;
			for (int i = -sincSize / 2; i < sincSize / 2; ++i)
			{
				sum += hist_[x++] * filter[sincSize / 2 + i];
				if (x == histSize)
				{
					x = sseCount;
				}
			}
#else
			// SSE
			const int numCoefs = sincSize; // coefs.numCoefs_;
			assert((numCoefs & 0x03) == 0); // factor of 4?

											// SSE intrinsics
			__m128 sum1 = _mm_setzero_ps();
			__m128* pCoefs = (__m128*) filter; // coefs.coefs_;

			const float* pSignal = &(hist_[readIndex_]);

			// Operate on as many as we can up to end of buffer (but no more than num coefs)
			int todo = (std::min)(numCoefs, histSize - readIndex_) & 0xfffffffc;

			for (int i = todo; i > 0; i -= 4)
			{
				// History samples are unaligned, hence _mm_loadu_ps().
				sum1 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(pSignal), *pCoefs++), sum1);
				pSignal += 4;
			}

			// Process any leftover from start of hist buffer. (wrap).
			pSignal -= histSize - sseCount;
			int remain = numCoefs - todo;
			for (; remain > 0; remain -= 4)
			{
				sum1 = _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(pSignal), *pCoefs++), sum1);
				pSignal += 4;
			}

			// Horizontal add.
			__m128 t = _mm_add_ps(sum1, _mm_movehl_ps(sum1, sum1));
			auto sum2 = _mm_add_ss(t, _mm_shuffle_ps(t, t, 1));
			float sum;
			_mm_store_ss(&sum, sum2);
#endif
			*to++ = sum;

			if (++subSample == oversampleFactor)
			{
				subSample = 0;
				++readIndex_;
				if (readIndex_ >= histSize)
				{
					readIndex_ -= histSize - sseCount;
				}
			}
		}
	}
};

class ug_oversampler_io :
	public ug_base
{
public:
	ug_oversampler_io();
	DECLARE_UG_BUILD_FUNC(ug_oversampler_io);
	UPlug* AddProxyPlug(UPlug* p);
	UPlug* GetProxyPlug(int i, UPlug* p);

	class ug_oversampler* oversampler_;
	int oversamplerBlockPos;
	int oversampleFactor_;

protected:
	int subSample;

	std::vector<UPlug *> OutsidePlugs;
	/* Silence detection. */
	std::vector<int> staticCount;
	std::vector<int> states;
	enum { OSP_Streaming, OSP_FilterSettling, OSP_StaticCount, OSP_Sleep};
};
