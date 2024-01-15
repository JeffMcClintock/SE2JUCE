#include "pch.h"
#include <algorithm>
#include "ug_math_base.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_math_base)

using namespace std;

namespace
{
REGISTER_MODULE_1(L"Subtract", IDS_MN_SUBTRACT,IDS_MG_MATH,ug_math_sub,CF_STRUCTURE_VIEW,L"Subtracts Input 2 from Input 1.");
REGISTER_MODULE_1(L"Multiply", IDS_MN_MULTIPLY,IDS_MG_MATH,ug_math_mult,CF_STRUCTURE_VIEW,L"Multiplys two Voltages.");
REGISTER_MODULE_1(L"Divide", IDS_MN_DIVIDE,IDS_MG_MATH,ug_math_div,CF_STRUCTURE_VIEW,L"Divides Input 1 by Input 2.");
}

bool ug_math_base::PPGetActiveFlag()
{
	// Only active if BOTH inputs are active
	return GetPlug(0)->GetPPDownstreamFlag() && GetPlug(1)->GetPPDownstreamFlag();  // Multiplier is only polyphonic if both inputs are downstream
}

void ug_math_base::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
	state_type input1_status = GetPlug( PN_INPUT1 )->getState();
	state_type input2_status = GetPlug( PN_INPUT2 )->getState();
	state_type out_stat = max( input1_status, input2_status );

	if( can_sleep_if_one_input_zero )
	{
		// if one input is zero (and not changing) no need to recalc output
		if( input1_status < ST_RUN && GetPlug( PN_INPUT1 )->getValue() == 0.0f )
			out_stat = ST_ONE_OFF;

		if( input2_status < ST_RUN && GetPlug( PN_INPUT2 )->getValue() == 0.0f )
			out_stat = ST_ONE_OFF;
	}

	if( out_stat < ST_RUN )
	{
		SET_CUR_FUNC( &ug_math_base::process_both_stop1 );
		ResetStaticOutput();
	}
	else
	{
		if( input1_status < ST_RUN )
		{
			SET_CUR_FUNC( &ug_math_base::process_B_run );
		}
		else
		{
			if( input2_status < ST_RUN )
			{
				SET_CUR_FUNC( &ug_math_base::process_A_run );
			}
			else
			{
				SET_CUR_FUNC( &ug_math_base::process_both_run );
			}
		}
	}

	GetPlug(PN_OUT)->TransmitState( p_clock, out_stat );
}

ug_math_base::ug_math_base() :
	can_sleep_if_one_input_zero(false)
{
	SetFlag(UGF_SSE_OVERWRITES_BUFFER_END);
}

// call the derived class once, to get it's output value
void ug_math_base::process_both_stop1(int start_pos, int sampleframes)
{
	assert(sampleframes > 0);

	process_A_run( start_pos, 1 ); // calculate one output value
	static_output_val = *(out1_ptr + start_pos);
	SET_CUR_FUNC( &ug_math_base::process_both_stop2 );

	if( --sampleframes > 0 )
	{
		start_pos++;
		process_both_stop2( start_pos, sampleframes);
	}
}

// output constant value from above
void ug_math_base::process_both_stop2(int start_pos, int sampleframes)
{
	const float out_val = static_output_val;
	float* out = out1_ptr + start_pos;

#ifndef GMPI_SSE_AVAILABLE

	// No SSE. Use C++ instead.
	for (int s = sampleframes; s > 0; s--)
	{
		*out++ = out_val;
	}

#else
	// Use SSE instructions.
	auto counter = sampleframes;

	// process fiddly non-sse-aligned prequel.
	while (counter > 0 && reinterpret_cast<intptr_t>(out) & 0x0f)
	{
		*out++ = out_val;
		--counter;
	}

	// SSE intrinsics
	const __m128 staticVal = _mm_set_ps1(out_val);
	__m128* pDest = (__m128*) out;

	while (counter > 0)
	{
		*pDest++ = staticVal;
		counter -= 4;
	}
#endif

	SleepIfOutputStatic(sampleframes);
}

//====================== SUBTRACT ======================================


// Fill an array of InterfaceObjects with plugs and parameters
IMPLEMENT_UG_INFO_FUNC2(ug_math_sub)

{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	LIST_PIN2( L"Input 1", in_ptr[0], DR_IN, L"8", L"", IO_POLYPHONIC_ACTIVE, L"Input 2 is Subtracted from Input 1");
	LIST_PIN2( L"Input 2", in_ptr[1], DR_IN, L"8", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN2( L"Output",  out1_ptr,DR_OUT, L"0", L"", 0, L"");
}

void ug_math_sub::process_both_run(int start_pos, int sampleframes)
{
	float* in1 = in_ptr[0] + start_pos;
	float* in2 = in_ptr[1] + start_pos;
	float* out = out1_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = *in1++ - *in2++;
	}
}

void ug_math_sub::process_A_run(int start_pos, int sampleframes)
{
	float* in1 = in_ptr[0] + start_pos;
	float* in2 = in_ptr[1] + start_pos;
	float* out = out1_ptr + start_pos;
	float gain = *in2;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = *in1++ - gain;
	}
}

void ug_math_sub::process_B_run(int start_pos, int sampleframes)
{
	float* in1 = in_ptr[0] + start_pos;
	float* in2 = in_ptr[1] + start_pos;
	float* out = out1_ptr + start_pos;
	float gain = *in1;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = gain - *in2++;
	}
}

//====================== MULTIPLY ======================================
IMPLEMENT_UG_INFO_FUNC2(ug_math_mult)

{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	LIST_PIN2( L"Input 1", in_ptr[0], DR_IN, L"8", L"", IO_LINEAR_INPUT, L"The 2 inputs are Multiplied");
	LIST_PIN2( L"Input 2", in_ptr[1], DR_IN, L"8", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN2( L"Output",  out1_ptr,DR_OUT, L"0", L"", 0, L"");
}

void ug_math_mult::process_both_run(int start_pos, int sampleframes)
{
	float* in1 = in_ptr[0] + start_pos;
	float* in2 = in_ptr[1] + start_pos;
	float* out = out1_ptr + start_pos;

#ifndef GMPI_SSE_AVAILABLE

	// No SSE. Use C++ instead.
	for (int s = sampleframes; s > 0; s--)
	{
		*out++ = 10.f * *in1++ * *in2++;
	}

#else
	// Use SSE instructions.

	// process fiddly non-sse-aligned prequel.
	while (sampleframes > 0 && reinterpret_cast<intptr_t>(out) & 0x0f)
	{
		*out++ = 10.0f * *in1++ * *in2++;
		--sampleframes;
	}

	// SSE intrinsics
	__m128* pIn1 = (__m128*) in1;
	__m128* pIn2 = (__m128*) in2;
	__m128* pDest = (__m128*) out;
	const __m128 staticTen = _mm_set_ps1( 10.0f );

	// move first runing input plus static sum.
	while (sampleframes > 0)
	{
		*pDest++ = _mm_mul_ps( _mm_mul_ps( *pIn1++, *pIn2++ ), staticTen );
		sampleframes -= 4;
	}

#endif
}

void ug_math_mult::process_A_run(int start_pos, int sampleframes)

{
	float* in1 = in_ptr[0] + start_pos;
	float* in2 = in_ptr[1] + start_pos;
	float* out = out1_ptr + start_pos;

	const float gain = *in2 * 10.0f;

#ifndef GMPI_SSE_AVAILABLE
	// No SSE. Use C++ instead.
	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = *in1++ * gain;
	}

#else
	// Use SSE instructions.
	// process fiddly non-sse-aligned prequel.
	while (sampleframes > 0 && reinterpret_cast<intptr_t>(out) & 0x0f)
	{
		*out++ = *in1++ * gain;
		--sampleframes;
	}

	// SSE intrinsics
	__m128* pIn1 = (__m128*) in1;
	__m128* pDest = (__m128*) out;
	const __m128 staticGain = _mm_set_ps1( gain );

	// move first runing input plus static sum.
	while (sampleframes > 0)
	{
		*pDest++ = _mm_mul_ps( *pIn1++, staticGain );
		sampleframes -= 4;
	}
#endif
}

void ug_math_mult::process_B_run(int start_pos, int sampleframes)
{
	float* in1 = in_ptr[0] + start_pos;
	float* in2 = in_ptr[1] + start_pos;
	float* out = out1_ptr + start_pos;
	const float gain = *in1 * 10.0f;

#ifndef GMPI_SSE_AVAILABLE

	// No SSE. Use C++ instead.
	for (int s = sampleframes; s > 0; s--)
	{
		*out++ = *in2++ * gain;
	}

#else

	// process fiddly non-sse-aligned prequel.
	while (sampleframes > 0 && reinterpret_cast<intptr_t>(out) & 0x0f)
	{
		*out++ = *in2++ * gain;
		--sampleframes;
	}

	// SSE intrinsics
	__m128* pIn2 = (__m128*) in2;
	__m128* pDest = (__m128*) out;
	const __m128 staticGain = _mm_set_ps1( gain );

	// move first runing input plus static sum.
	while (sampleframes > 0)
	{
		*pDest++ = _mm_mul_ps( *pIn2++, staticGain );
		sampleframes -= 4;
	}
#endif
}

ug_math_mult::ug_math_mult()
{
	can_sleep_if_one_input_zero = true;
	//SetFlag(UGF_SSE_OVERWRITES_BUFFER_END;
	SetFlag(UGF_SSE_OVERWRITES_BUFFER_END);
}

//====================== DIVIDE ======================================
IMPLEMENT_UG_INFO_FUNC2(ug_math_div)

{
	// IO Var, Direction, Datatype, Name, Default, defid (index into unit_gen::PlugFormats)
	LIST_PIN2( L"Input 1", in_ptr[0], DR_IN, L"8", L"", IO_LINEAR_INPUT, L"Input 1 is Divided by Input 2");
	LIST_PIN2( L"Input 2", in_ptr[1], DR_IN, L"8", L"", IO_POLYPHONIC_ACTIVE, L"");
	LIST_PIN2( L"Output",  out1_ptr,DR_OUT, L"0", L"", 0, L"");
}

void ug_math_div::process_both_run(int start_pos, int sampleframes)
{
	float* in1 = in_ptr[0] + start_pos;
	float* in2 = in_ptr[1] + start_pos;
	float* out = out1_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = 0.1f * *in1++ / *in2++;
	}
}

void ug_math_div::process_A_run(int start_pos, int sampleframes)
{
	float* in1 = in_ptr[0] + start_pos;
	float* in2 = in_ptr[1] + start_pos;
	float* out = out1_ptr + start_pos;
	const float gain = 0.1f / *in2;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = *in1++ * gain;
	}
}

void ug_math_div::process_B_run(int start_pos, int sampleframes)
{
	float* in1 = in_ptr[0] + start_pos;
	float* in2 = in_ptr[1] + start_pos;
	float* out = out1_ptr + start_pos;
	const float gain = 0.1f * *in1;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		*out++ = gain / *in2++;
	}
}

