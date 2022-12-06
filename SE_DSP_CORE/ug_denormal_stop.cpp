#include "pch.h"

#include "ug_denormal_stop.h"

#include <math.h>

#include "module_register.h"

SE_DECLARE_INIT_STATIC_FILE(ug_denormal_stop)

namespace

{

REGISTER_MODULE_1(L"Denormal Cleaner", IDS_MN_DENORMAL_CLEANER,IDS_MG_DIAGNOSTIC,ug_denormal_stop,CF_STRUCTURE_VIEW,L"Cleans DeNormal numbers any audio passing through.  Denormal numbers are VERY small value that are inaudible, but waste CPU resources.  This module is no longer needed, now that denormal removal is built-in to many other modules.");

}



void ug_denormal_stop::sub_process(int start_pos, int sampleframes)

{
	// treat floats as ints, read bit fiields directly
	unsigned int* in  = (unsigned int*)(in1_ptr + start_pos);
	unsigned int* out = (unsigned int*)(out1_ptr + start_pos);

	for( int s = sampleframes ; s > 0 ; s-- )
	{
		unsigned int l = *in++;

		// include 'cleaning' of -ve zero, seems some people may regard it as 'denormal', removed condition (o != 0.f)
		if( ((l & 0x7FF00000) == 0) ) // anything less than approx 1E-38 (!!should be 7f8?? not sure)
		{
			l = 0;
		}

		*out++ = l;
	}
}



