#include "./PatchMemoryInt.h"

SE_DECLARE_INIT_STATIC_FILE(PatchMemoryInt);

REGISTER_PLUGIN( PatchMemoryInt, L"SE PatchMemory Int" );
REGISTER_PLUGIN( PatchMemoryInt, L"SE PatchMemory Int Out" );

PatchMemoryInt::PatchMemoryInt(IMpUnknown* host) : MpBase(host)
{
	initializePin( 0, pinValueIn );
	initializePin( 1, pinValueOut );
}

void PatchMemoryInt::onSetPins()  // one or more pins_ updated.  Check pin update flags to determin which ones.
{
	if( pinValueIn.isUpdated() )
	{
		pinValueOut = pinValueIn;
	}

	// now automatic. setSleep(true);
}


