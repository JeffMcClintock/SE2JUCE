#include "./PatchMemoryBool.h"

SE_DECLARE_INIT_STATIC_FILE(PatchMemoryBool);
SE_DECLARE_INIT_STATIC_FILE(PatchMemoryBoolOut);

REGISTER_PLUGIN( PatchMemoryBool, L"SE PatchMemory Bool" );
REGISTER_PLUGIN( PatchMemoryBool, L"SE PatchMemory Bool Out" );

PatchMemoryBool::PatchMemoryBool(IMpUnknown* host) : MpBase(host)
{
	initializePin( 0, pinValueIn );
	initializePin( 1, pinValueOut );
}

void PatchMemoryBool::onSetPins()  // one or more pins_ updated.  Check pin update flags to determin which ones.
{
	if( pinValueIn.isUpdated() )
	{
		pinValueOut = pinValueIn;
	}

	// now automatic. setSleep(true);
}







