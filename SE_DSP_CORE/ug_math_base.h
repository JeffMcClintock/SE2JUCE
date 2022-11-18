#pragma once
#include "ug_base.h"
#include "modules/shared/xp_simd.h"

#define PN_INPUT1	0
#define PN_INPUT2	1
#define PN_OUT		2

class ug_math_base : public ug_base
{
public:
	ug_math_base();
	virtual void process_both_run(int start_pos, int sampleframes) = 0;
	virtual void process_A_run(int start_pos, int sampleframes) = 0;
	virtual void process_B_run(int start_pos, int sampleframes) = 0;
	void process_both_stop1(int start_pos, int sampleframes);
	void process_both_stop2(int start_pos, int sampleframes);
	virtual void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

	void process_bypass(int start_pos, int sampleframes)
	{
		const float* in = in_ptr[bypassPlug] + start_pos;
		float* out = out1_ptr + start_pos;

#ifndef GMPI_SSE_AVAILABLE

		// No SSE. Use C++ instead.
		for (int s = sampleframes; s > 0; s--)
		{
			*out++ = *in++;
		}
#else
//		apex::memcpy(out, in, sampleframes * sizeof(*in));

		// Use SSE instructions.
#if 1
		// process fiddly non-sse-aligned prequel.
		while (reinterpret_cast<intptr_t>(out) & 0x0f)
		{
			*out++ = *in++;
			--sampleframes;
		}

		// SSE intrinsics
		const __m128* pIn1 = (__m128*) in;
		__m128* pDest = (__m128*) out;

		while (sampleframes > 0)
		{
			*pDest++ = *pIn1++;
			sampleframes -= 4;
		}
#else

#if 0
		// more asm instructions in loop.
		int fiddlyCount = 4 - (( reinterpret_cast<intptr_t>(out) >> 2) & 0x03);
		sampleframes -= fiddlyCount;
		_mm_storeu_ps(out, _mm_loadu_ps(in));

		in  += fiddlyCount; //reinterpret_cast<float*>(reinterpret_cast<intptr_t>(in  + 3) & 0x0f);
		out += fiddlyCount; //= reinterpret_cast<float*>(reinterpret_cast<intptr_t>(out + 3) & 0x0f);
		assert((reinterpret_cast<intptr_t>(out) & 0x0f) == 0);

		while (sampleframes > 0)
		{
			_mm_store_ps(out, _mm_load_ps(in));
			in += 4;
			out += 4;
			sampleframes -= 4;
		}

#else
		// proces initial samples with unaligned load and store, then align pointers and process remaining.
		// equal performance, more asm instructions, harder to understand.
		auto end = in + sampleframes;

		// Process potentially unalligned prequel
		_mm_storeu_ps(out, _mm_loadu_ps(in));

		// Increment and align pointers.
		constexpr intptr_t mask = ((intptr_t)-1) ^ 0x0f;
		in = reinterpret_cast<const float*>(reinterpret_cast<intptr_t>(in + 4) & mask);
		out = reinterpret_cast<float*>(reinterpret_cast<intptr_t>(out + 4) & mask);

		// Process aligned remainder.
		int count = (3 + (int)(end - in)) >> 2;
		while (count > 0)
		{
			_mm_store_ps(out, _mm_load_ps(in));
			in += 4;
			out += 4;
			--count;
		}
#endif

#endif
#endif
	}

	bool PPGetActiveFlag() override;

protected:
	float* in_ptr[2];
	float* out1_ptr;
	bool can_sleep_if_one_input_zero;
	float static_output_val;
	int bypassPlug = -1;
};

class ug_math_div : public ug_math_base

{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_math_div);

	void process_both_run(int start_pos, int sampleframes) override;
	void process_A_run(int start_pos, int sampleframes) override;
	void process_B_run(int start_pos, int sampleframes) override;
};

class ug_math_mult : public ug_math_base
{
public:
	ug_math_mult();

	void process_both_run(int start_pos, int sampleframes) override;
	void process_A_run(int start_pos, int sampleframes) override;
	void process_B_run(int start_pos, int sampleframes) override;

	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_math_mult);
};

class ug_math_sub : public ug_math_base
{
public:
	void process_both_run(int start_pos, int sampleframes) override;
	void process_A_run(int start_pos, int sampleframes) override;
	void process_B_run(int start_pos, int sampleframes) override;

	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_math_sub);
};
