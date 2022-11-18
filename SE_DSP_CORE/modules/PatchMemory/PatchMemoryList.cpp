#include "./PatchMemoryList.h"

REGISTER_PLUGIN( PatchMemoryList, L"SE PatchMemory List3" );
SE_DECLARE_INIT_STATIC_FILE(PatchMemoryList3);

PatchMemoryList::PatchMemoryList(IMpUnknown* host) : MpBase(host)
{
	initializePin( pinValueIn );
	initializePin( pinValueOut );
}

void PatchMemoryList::onSetPins()  // one or more pins_ updated.  Check pin update flags to determin which ones.
{
	pinValueOut = (int) pinValueIn;
}







