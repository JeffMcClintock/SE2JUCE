#include "pch.h"
#include "ug_denormal_detect.h"
#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_denormal_detect)

namespace

{
REGISTER_MODULE_1_BC(105,L"Denormal Detector", IDS_MN_DENORMAL_DETECTOR,IDS_MG_DIAGNOSTIC,ug_denormal_detect,CF_STRUCTURE_VIEW,L"A diagnostic module to indicate DeNormal numbers in the audio.  Connect to a LED for visual indication.  Denormal numbers are VERY small values that are inaudible, but waste CPU resources.");
}

void ug_denormal_detect::sub_process(int start_pos, int sampleframes)
{
	// treat floats as ints, read bit fields directly
	unsigned int* in  = (unsigned int*)(in1_ptr + start_pos);
	float* out = out1_ptr + start_pos;

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		unsigned int l = *in++;
		float o;

		if( (l & 0x7FF00000) == 0 && (l & 0x000FFFFF) != 0 ) // anything less than approx 1E-38 excluding +ve and -ve zero (two distinct values)
		{
			o = 5.f;
		}
		else
		{
			o = 0.f;
		}

		*out++ = o;
	}
}



