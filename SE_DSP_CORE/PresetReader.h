#pragma once

template<typename T, typename FUNC>
void ParseXmlPreset(T parameter_xml, FUNC callback)
{
	const auto empty = "";
	int voiceId = 0;

	for (auto patch_xml = parameter_xml->FirstChildElement("patch-list"); patch_xml; patch_xml = patch_xml->NextSiblingElement())
	{
		// new format
		int preset = 0;
		for (auto val_xml = patch_xml->FirstChildElement("s"); val_xml; val_xml = val_xml->NextSiblingElement())
		{
			const auto xmlvalue = val_xml->GetText();
			int repeat = 1;
			val_xml->QueryIntAttribute("repeat", &repeat);

			for (int i = 0; i < repeat; ++i)
			{
				callback(
					voiceId,
					preset++,
					xmlvalue ? xmlvalue : empty // protect against nullptr
				);
			}
		}

		// old format
		if (preset == 0 && patch_xml->GetText())
		{
			// FAILS BADLY on single apostrophes, avoid.
			std::vector<std::string> presetValues;
			XmlSplitString(patch_xml->GetText(), presetValues);

			for (auto& xmlvalue : presetValues)
			{
				callback(voiceId, preset++, xmlvalue.c_str());
			}
		}

		++voiceId;
	}
}