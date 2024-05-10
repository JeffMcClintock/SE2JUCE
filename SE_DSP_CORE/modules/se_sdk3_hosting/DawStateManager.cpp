#include "DawStateManager.h"
#include "../../tinyxml/tinyxml.h"
#include "RawConversions.h"
#include "PresetReader.h"
#include "HostControls.h"

DawPreset const* DawStateManager::fileToPreset(const char* filename, int presetIndex)
{
	TiXmlDocument doc;
	doc.LoadFile(filename);

	if (doc.Error())
	{
		assert(false);
		return {};
	}

	TiXmlHandle hDoc(&doc);

	return retainPreset(new DawPreset(parametersInfo, hDoc.ToNode(), presetIndex));
}

DawPreset const* DawStateManager::xmlToPreset(std::string xml, bool retain)
{
	if(xml.empty())
		return {};

	assert(!parametersInfo.empty()); // need to initialise first

	auto preset = new DawPreset(parametersInfo, xml);

	if(preset->hash == 0 && preset->name.empty()) // failed to read preset
	{
		delete preset;
		return {};
	}

	if(retain)
		retainPreset(preset);

	return preset;
}

void DawStateManager::setPresetFromUnownedPtr(DawPreset const* preset)
{
	setPreset(retainPreset(new DawPreset(*preset)));
}

