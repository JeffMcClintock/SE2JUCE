#pragma once
#include <string>

// VST3 preset support.

/*
#include "VstPreset.h"
*/

namespace Steinberg
{
	class FUID;
}

namespace VstPresetUtil
{
	void WritePreset(std::wstring filename, std::string categoryName, std::string vendorName, std::string productName, Steinberg::FUID *processorId, std::string xmlPreset);

	std::string ReadPreset(std::wstring filename, std::string* returnCategory = nullptr);
}
