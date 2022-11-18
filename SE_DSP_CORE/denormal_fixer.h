#pragma once
#include <float.h>

/* 
#include "denormal_fixer.h"
	DenormalFixer flushDenormals;
*/

class DenormalFixer
{
public:
	DenormalFixer()
	{
		// On waves, they already set denormal-flush. So OK either way.
		#if defined( _MSC_VER )
			_controlfp_s( &fpState, 0, 0 ); // Store original mode.
			_controlfp_s( 0, _DN_FLUSH, _MCW_DN ); // flush-denormals-to-zero mode.
		#endif
	}
	~DenormalFixer()
	{
		#if defined( _MSC_VER )
			_controlfp_s( 0, fpState, _MCW_DN ); // restore mode
		#endif
	}

private:
	unsigned int fpState;
};