#include "pch.h"
#include "math.h"
#include "ug_math_ceil.h"
#include "module_register.h"
#include "./modules/shared/xplatform.h"
#include "./modules/shared/xp_simd.h"

SE_DECLARE_INIT_STATIC_FILE(ug_math_ceil)

namespace
{
REGISTER_MODULE_1(L"Ceil", IDS_MN_CEIL,IDS_MG_MATH,ug_math_ceil,CF_STRUCTURE_VIEW,L"Outputs the next highest whole number");
}

void ug_math_ceil::sub_process(int start_pos, int sampleframes)

{
	float* in = in1_ptr + start_pos;
	float* out = out1_ptr + start_pos;

	// Attempt to handle number rounded just under a whole number. Not very good.
	const float very_small_number = 6e-7f;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		// C++. OK , but slow.
		// *out++ = 0.1f * ceilf( (*in++) * 10.0f );

		float temp = *in * 10.0f + very_small_number;
		if( temp < 0.0f )
		{
//			*out++ = 0.1f * (float)FastRealToIntTruncateTowardZero(temp)); // fast float-to-int using SSE. truncation toward zero.
			*out++ = 0.1f * (float)FastRealToIntTruncateTowardZero(temp);
		}
		else
		{
//			*out++ = 0.1f * (1.0f + (float) FastRealToIntTruncateTowardZero(temp)) );
			*out++ = 0.1f * (1.0f + (float)FastRealToIntTruncateTowardZero(temp));
		}
		++in;
	}
}
