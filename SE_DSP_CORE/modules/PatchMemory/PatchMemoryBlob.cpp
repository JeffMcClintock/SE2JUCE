#include "./PatchMemoryBlob.h"

REGISTER_PLUGIN( PatchMemoryBlob, L"SE PatchMemory Blob" );
REGISTER_PLUGIN( PatchMemoryBlob, L"SE PatchMemory Blob Out" );

PatchMemoryBlob::PatchMemoryBlob(IMpUnknown* host) : MpBase(host)
{
	initializePin( 0, pinValueIn );
	initializePin( 1, pinValueOut );
}

void PatchMemoryBlob::onSetPins()  // one or more pins_ updated.  Check pin update flags to determin which ones.
{
	if( pinValueIn.isUpdated() )
	{
		pinValueOut = pinValueIn;
	}
}







