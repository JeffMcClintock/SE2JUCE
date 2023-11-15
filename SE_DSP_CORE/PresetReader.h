#pragma once
/*
// .synthedit file
						<param type="6" handle="1482928534" shortName="VCA Volume" vstParameterNumber="1" module="318520817" moduleParamId="0">
							<patch-list>
								<s>7.48</s>		<- preset 0 voice 0
								<s>6.2</s>		<- preset 1 voice 0
								<s>6.335</s>
								<s>0.2518</s>
								<s>5.12</s>
								<s repeat="123">8</s>
							</patch-list>
							<patch-list Voice="1">
								<s>7.48</s>
								<s>6.2</s>
								<s>6.335</s>
								<s>0.2518</s>
								<s>5.12</s>
								<s repeat="123">8</s>
							</patch-list>
						</param>





FORMAT 1
<Preset pluginId="4f505055" name="Matrix Softstrings II">
 <Param id="53" val="6.55" MIDI="33554509" />

FORMAT 1B
<Preset pluginId="4f505055" name="Matrix Softstrings II">
 <Param id="53" MIDI="33554509" >
   1.1, 2.2, 3.3 ...
 <Param/>
 
FORMAT 2 (in the plugin 's' is always a voice, in the project file it's a preset.
<Preset pluginId="4f505055" name="Matrix Softstrings II">
 <Param id="123" val="97.109">
  <patch-list>                                         <- preset 0
   <s>97.109</s>                                         <- voice 0
   <s>97.109</s>                                         <- voice 1
   <s>97.109</s>                                         <- voice 2
   <s repeat="2">97.109</s>                              <- voice 3,4
   ...
  </patch-list>
 </Param>
*/

// *.synthedit documents only, where repeated <s> means many presets
template<typename T, typename FUNC>
void ParseXmlPreset(T parameter_xml, FUNC callback)
{
	const auto empty = "";
	int voiceId = 0;

	auto patch_xml = parameter_xml->FirstChildElement("patch-list");

	if (patch_xml)
	{
		// format 1 and 2
		for (; patch_xml; patch_xml = patch_xml->NextSiblingElement())
		{
			// format 2 (faulty, confuses voice and preset)
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

			// format 1, single text block comma-separated
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
	else
	{
		// fallback to 'val' attribute
		if (const auto v = parameter_xml->Attribute("val"); v)
		{
			callback(voiceId, 0, v);
		}
	}
}

// for plugins, with always a single preset, and where repeated <s> means many voices
template<typename T, typename FUNC>
void ParseXmlPreset2(T parameter_xml, FUNC callback)
{
	const auto empty = "";

	// format 1 and 2
	auto patch_xml = parameter_xml->FirstChildElement("patch-list");
	assert(!patch_xml || !patch_xml->NextSiblingElement()); // one preset ONLY

	if (patch_xml)
	{
		// format 2
		int voiceId = 0;
		for (auto val_xml = patch_xml->FirstChildElement("s"); val_xml; val_xml = val_xml->NextSiblingElement())
		{
			const auto xmlvalue = val_xml->GetText();
			int repeat = 1;
			val_xml->QueryIntAttribute("repeat", &repeat);

			for (int i = 0; i < repeat; ++i)
			{
				callback(
					voiceId++,
					xmlvalue ? xmlvalue : empty // protect against nullptr
				);
			}
		}

		// format 1, single text block comma-separated
		if (patch_xml->GetText())
		{
			// FAILS BADLY on single apostrophes, avoid.
			std::vector<std::string> presetValues;
			XmlSplitString(patch_xml->GetText(), presetValues);

			for (auto& xmlvalue : presetValues)
			{
				callback(voiceId, xmlvalue.c_str());
			}
		}
	}
	else
	{
		// fallback to 'val' attribute
		if(const auto v = parameter_xml->Attribute("val"); v)
		{
			constexpr int voiceId = 0;
			callback(voiceId, v);
		}	
	}
}