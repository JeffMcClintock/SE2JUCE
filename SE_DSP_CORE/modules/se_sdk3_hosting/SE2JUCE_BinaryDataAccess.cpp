#include <JuceHeader.h>
#include "BinaryData.h"

extern const char* se2juce_getNamedResource(const char* name, int& returnBytes)
{
	returnBytes = 0; // defensive

	return BinaryData::getNamedResource(name, returnBytes);
}

#if 0
extern const char* se2juce_getIndexedResource(int index, int& returnBytes)
{
	returnBytes = 0; // defensive

	if (index < 0 || BinaryData::namedResourceListSize >= index)
		return {};

	return BinaryData::getNamedResource(BinaryData::namedResourceList[index], returnBytes);
}
#endif

// see also SE_DECLARE_INIT_STATIC_FILE
#define INIT_STATIC_FILE(filename) void se_static_library_init_##filename(); se_static_library_init_##filename();

extern void initialise_synthedit_extra_modules(bool passFalse)
{
#ifndef _DEBUG
	// NOTE: We don't have to actually call these functions (they do nothing), only *reference* them to
	// cause them to be linked into plugin such that static objects get initialised.
	if (!passFalse)
		return;
#endif

	INIT_STATIC_FILE(Slider_Gui);
}