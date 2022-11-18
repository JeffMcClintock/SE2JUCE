#include "./PatchMemoryText.h"

SE_DECLARE_INIT_STATIC_FILE(PatchMemoryText)
SE_DECLARE_INIT_STATIC_FILE(PatchMemoryTextOut)

REGISTER_PLUGIN( PatchMemoryText, L"SE PatchMemory Text2" );
REGISTER_PLUGIN( PatchMemoryText, L"SE PatchMemory Text Out" );

PatchMemoryText::PatchMemoryText(IMpUnknown* host) : MpBase(host)
{
	initializePin( 0, pinValueIn );
	initializePin( 1, pinValueOut );
}

void PatchMemoryText::onSetPins()  // one or more pins_ updated.  Check pin update flags to determin which ones.
{
	if( pinValueIn.isUpdated() )
	{
		pinValueOut = pinValueIn;
	}

	// now automatic. setSleep(true);
}







