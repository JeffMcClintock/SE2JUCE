#pragma once
#include <string>
#include "modules/shared/platform_string.h"

// VST2 preset support.

/*
#include "Vst2Preset.h"
*/

namespace Vst2PresetUtil
{
	struct wrapperHeader
	{
		// VST2 Wrapper. Processor and Controler size 64-bit.
		int64_t sizeA; // processor xml size (64-bit). Sum of 32-bit SE chunk size and chunk data length.
		int64_t sizeB; // controller xml (unused)

		// SynthEdit chunk size 32-bit 
		int32_t sizeC; // processor xml size (32-bit)
		char xml[1];
	};

	void WritePreset(platform_string wfilename, int32_t Vst2Id, std::string presetName, const std::string& xmlPreset);
	std::string ReadPreset(platform_string wfilename, int32_t Vst2Id_32bit, int32_t Vst2Id_64bit);
}
