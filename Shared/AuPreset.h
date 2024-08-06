#pragma once
#include <string>

/*
#include "AuPreset.h"
*/

// *.aupreset file are stored under [Home]/Library/Audio/Presets/[manufacturer]/[plug-in]/.
namespace AuPresetUtil
{
	bool WritePreset(
        std::wstring filename,
        const std::string& presetName,
        std::string pluginType,
        std::string unique_manufacturer_id,
        std::string vst_4_Char_id,
        std::string xmlPreset
    );

	std::string ReadPreset(std::wstring filename);
}
