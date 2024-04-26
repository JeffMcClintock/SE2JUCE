#include "DawStateManager.h"
#include "../../tinyxml/tinyxml.h"
#include "RawConversions.h"
#include "PresetReader.h"
#include "HostControls.h"

void DawStateManager::setPreset(const std::string& presetString)
{
	// perform parsing on callers thread.
	setPreset(retainPreset(new DawPreset(parameterInfos, presetString)));

	/* no, needed by undo manager
	// remove old presets, assuming not accessed anywhere. could use shared_ptr? maybe
	if (presets.size() > 10)
	{
		presets.erase(presets.begin());
	}
	*/
	
}

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

	return retainPreset(new DawPreset(parameterInfos, hDoc.ToNode(), presetIndex));
}

void DawStateManager::setPreset(DawPreset const* preset)
{
	preset->ignoreProgramChangeActive = ignoreProgramChange;

	for (auto& cb : callbacks)
	{
		cb(preset);
	}

	// update atomics
}

DawPreset const* DawStateManager::retainPreset(DawPreset const* preset)
{
	// retain immutable preset
	const std::lock_guard<std::mutex> lock{ presetMutex };

//	preset->idx = ++currentPresetIndex;
	presets.push_back(std::unique_ptr<const DawPreset>(preset));

	return preset;
}

DawPreset::DawPreset(const DawPreset& other)
{
	name = other.name;
	hash = other.hash;
	params = other.params;
	category = other.category;
}

DawPreset::DawPreset(const std::map<int32_t, paramInfo>& parametersInfo, std::string presetString, int presetIdx) // presetIdx: in the case where file contains many.
{
	TiXmlDocument doc;
	doc.Parse(presetString.c_str());

	if (doc.Error())
	{
		assert(false);
		return;
	}

	initFromXML(parametersInfo, &doc, presetIdx);
}

DawPreset::DawPreset(const std::map<int32_t, paramInfo>& parameters, TiXmlNode* presetXml, int presetIdx)
{
	initFromXML(parameters, presetXml, presetIdx);
}

void DawPreset::initFromXML(const std::map<int32_t, paramInfo>& parametersInfo, TiXmlNode* parentXml, int presetIdx)
{
	/* Possible formats.
		1) PatchManager/Parameters/Parameter/Preset/patch-list
		2)      Presets/Parameters/Parameter/Preset/patch-list
		3) Presets/Preset/Param.val
		4)     Parameters/Param.val
		5)         Preset/Param.val
	*/

	TiXmlNode* presetXml = nullptr;
	auto presetsXml = parentXml->FirstChild("Presets");
	if (presetsXml) // exported from SE has Presets/Preset
	{
		int presetIndex = 0;
		for (auto node = presetsXml->FirstChild("Preset"); node; node = node->NextSibling("Preset"))
		{
			presetXml = node;
			if (presetIndex++ >= presetIdx)
				break;
		}
	}
	else
	{
		// Individual preset.
		presetXml = parentXml->FirstChildElement("Parameters"); // Format 4. "Parameters" in place of "Preset".
		if (!presetXml)
		{
			presetXml = parentXml->FirstChildElement("Preset"); // Format 5. (VST2 fxp preset).
		}
	}

	TiXmlNode* parametersE = nullptr;
	if (presetXml)
	{
		// Individual preset has Preset.Param.val or Parameters.Param.val
		parametersE = presetXml;
	}
	else
	{
		auto controllerE = parentXml->FirstChild("Controller");

		// should always have a valid root but handle gracefully if it does
		if (!controllerE)
			return;

		auto patchManagerE = controllerE->FirstChildElement();
		assert(strcmp(patchManagerE->Value(), "PatchManager") == 0);

		// Internal preset has parameters wrapping preset values. (PatchManager.Parameters.Parameter.patch-list)
		parametersE = patchManagerE->FirstChild("Parameters")->ToElement();

		// Could be either of two formats. Internal (PatchManager.Parameter.patch-list), or vstpreset V1.3 (Parameters.Param)
		auto ParamElement13 = parametersE->FirstChildElement("Param"); /// is this a V1.3 vstpreset?
		if (ParamElement13)
		{
			presetXml = parametersE;
		}
	}

	if (parametersE == nullptr)
		return;

// TODO!!!! ?????	hash = std::hash<std::string>{}(presetString);

	if (presetXml)
	{
		auto presetXmlElement = presetXml->ToElement();

		// Query plugin's 4-char code. Presence Indicates also that preset format supports MIDI learn.
		int32_t fourCC = -1;
		int formatVersion = 0;
		{
			std::string hexcode;
			if (TIXML_SUCCESS == presetXmlElement->QueryStringAttribute("pluginId", &hexcode))
			{
				formatVersion = 1;
				try
				{
					fourCC = std::stoul(hexcode.c_str(), nullptr, 16);
				}
				catch (...)
				{
					// who gives a f*ck
				}
			}
		}

		// !!! TODO: Check 4-char ID correct.
		//const int voiceId = 0;

		presetXmlElement->QueryStringAttribute("category", &category);
		presetXmlElement->QueryStringAttribute("name", &name);

		// assuming we are passed "Preset.Parameters" node.
		for (auto ParamElement = presetXml->FirstChildElement("Param"); ParamElement; ParamElement = ParamElement->NextSiblingElement("Param"))
		{
			int paramHandle = -1;
			ParamElement->QueryIntAttribute("id", &paramHandle);

			assert(paramHandle != -1);

			// ignore unrecognized parameters
			auto it = parametersInfo.find(paramHandle);
			if (it == parametersInfo.end())
				continue;

			auto& info = (*it).second;

//??			if (info.ignoreProgramChange)
//				continue;

			auto& values = params[paramHandle];
			/* no stateful paramters in preset we assume
						if (!parameter.stateful_) // For VST2 wrapper aeffect ptr. prevents it being inadvertantly zeroed.
							continue;

						// possibly need to handle elsewhere
						if (parameter.ignorePc_ && ignoreProgramChange) // a short time period after plugin loads, ignore-PC parameters will no longer overwrite existing value.
							continue;

			*/
			const std::string v = ParamElement->Attribute("val"); // todo instead of constructing a pointless string, what about just passing the char* to ParseToRaw()?

			values.dataType = info.dataType;
			values.rawValues_.push_back(ParseToRaw((int) info.dataType, v));

			// This block seems messy. Should updating a parameter be a single function call?
						// (would need to pass 'updateProcessor')
			{
				// calls controller_->updateGuis(this, voice)
//				parameter->setParameterRaw(gmpi::MP_FT_VALUE, (int32_t)raw.size(), raw.data(), voiceId);

				/* todo elsewhere
				// updated cached value.
				parameter->upDateImmediateValue();
				*/
			}

			// MIDI learn. part of a preset?????
			if (formatVersion > 0)
			{
				// TODO
				int32_t midiController = -1;
				ParamElement->QueryIntAttribute("MIDI", &midiController);

				std::string sysexU;
				ParamElement->QueryStringAttribute("MIDI_SYSEX", &sysexU);
			}
		}
	}
	else // Old-style. 'Internal' Presets
	{
		// assuming we are passed "PatchManager.Parameters" node.
		for (auto ParamElement = parametersE->FirstChildElement("Parameter"); ParamElement; ParamElement = ParamElement->NextSiblingElement("Parameter"))
		{
			// VST3 "Controler" XML uses "Handle", VST3 presets use "id" for the same purpose.
			int paramHandle = -1;
			ParamElement->QueryIntAttribute("Handle", &paramHandle);

			// ignore unrecognized parameters
			auto it = parametersInfo.find(paramHandle);
			if (it == parametersInfo.end())
				continue;

			auto& info = (*it).second;

			if(info.ignoreProgramChange)
				continue;

			auto& values = params[paramHandle];
			values.dataType = info.dataType;

			// Preset values from patch list.
			ParseXmlPreset(
				ParamElement,
				[values, info](int /*voiceId*/, int /*preset*/, const char* xmlvalue) mutable
				{
					values.rawValues_.push_back(ParseToRaw((int) info.dataType, xmlvalue));
					//						const auto raw = ParseToRaw(parameter.dataType, xmlvalue);
					//						if (parameter->setParameterRaw(gmpi::MP_FT_VALUE, (int32_t)raw.size(), raw.data(), voiceId))
					{
						// updated cached value.
//							parameter->upDateImmediateValue();
//							parameter->updateProcessor(gmpi::MP_FT_VALUE, voiceId);
					}
				}
			);
		}
	}

	// set any parameters missing from the preset to their default
	for (auto& [paramHandle, info] : parametersInfo)
	{
		if (params.find(paramHandle) == params.end())
		{
			auto& values = params[paramHandle];

			values.dataType = info.dataType;
			values.rawValues_ = info.defaultRaw;
		}
	}
}

std::string DawPreset::toString(int32_t pluginId, std::string presetNameOverride) const
{
	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "", "");
	doc.LinkEndChild(decl);

	auto element = new TiXmlElement("Preset");
	doc.LinkEndChild(element);

	{
		char buffer[20];
		sprintf(buffer, "%08x", pluginId);
		element->SetAttribute("pluginId", buffer);
	}

	if (!presetNameOverride.empty())
	{
		element->SetAttribute("name", presetNameOverride);
	}
	else if (!name.empty())
	{
		element->SetAttribute("name", name);
	}

	if (!category.empty())
		element->SetAttribute("category", category);

	for (auto& [handle, parameter] : params)
	{
		auto paramElement = new TiXmlElement("Param");
		element->LinkEndChild(paramElement);
		paramElement->SetAttribute("id", handle);

		const int voice = 0;
		auto& raw = parameter.rawValues_[voice];

		const auto val = RawToUtf8B((int) parameter.dataType, raw.data(), raw.size());

		paramElement->SetAttribute("val", val);

#if 0  // TODO??
		// MIDI learn.
		if (parameter->MidiAutomation != -1)
		{
			paramElement->SetAttribute("MIDI", parameter->MidiAutomation);

			if (!parameter->MidiAutomationSysex.empty())
				paramElement->SetAttribute("MIDI_SYSEX", WStringToUtf8(parameter->MidiAutomationSysex));
		}
#endif
	}

	TiXmlPrinter printer;
	printer.SetIndent(" ");
	doc.Accept(&printer);

	return printer.CStr();
}

void DawStateManager::init(TiXmlElement* parameters_xml)
{
	const std::lock_guard<std::mutex> lock{ presetMutex };

	// int strictIndex = 0;
	for (auto parameter_xml = parameters_xml->FirstChildElement("Parameter"); parameter_xml; parameter_xml = parameter_xml->NextSiblingElement("Parameter"))
	{
		int ParameterHandle = -1;
		parameter_xml->QueryIntAttribute("Handle", &ParameterHandle);

		int stateful_ = 1;
		parameter_xml->QueryIntAttribute("persistant", &stateful_);

		if (ParameterHandle == -1 || !stateful_) // assumes that non-statful parameters are not native parameters?
		{
			continue;
		}

		parameterInfos[ParameterHandle] = {};
		paramInfo& p = parameterInfos[ParameterHandle];

		//??		seParameter.strictIndex = strictIndex++;

		//		seParameter.dataType = DT_FLOAT;
		int ParameterTag = -1;
		int Private = 0;

#ifdef _DEBUG
		p.name = parameter_xml->Attribute("Name");
#endif
		{
			int temp{ (int) gmpi::PinDatatype::Float32};
			parameter_xml->QueryIntAttribute("ValueType", &temp);
			p.dataType = static_cast<gmpi::PinDatatype>(temp);

			// wheras pin enum values are 16-bit, enum parameters use 32-bit ints. otherwise ParseToRaw returns only 2-bytes
			if(p.dataType == gmpi::PinDatatype::Enum)
			{
				// VST and AU can't handle this type of parameter.
				p.dataType = gmpi::PinDatatype::Int32;
			}
		}
		parameter_xml->QueryIntAttribute("Index", &ParameterTag);
		parameter_xml->QueryIntAttribute("Private", &Private);
		p.private_ = Private != 0;

		if (p.dataType == gmpi::PinDatatype::String || p.dataType == gmpi::PinDatatype::Blob)
		{
			Private = 1; // VST and AU can't handle this type of parameter.
		}
		else
		{
			if (Private != 0)
			{
				// Check parameter is numeric and a valid type.
				assert(p.dataType == gmpi::PinDatatype::Enum || p.dataType == gmpi::PinDatatype::Float64 || p.dataType == gmpi::PinDatatype::Bool || p.dataType == gmpi::PinDatatype::Float32 || p.dataType == gmpi::PinDatatype::Int32 || p.dataType == gmpi::PinDatatype::Int64);
			}
		}

		int hostControl = -1;
		parameter_xml->QueryIntAttribute("HostControl", &hostControl);
		int ignorePc = 0;
		parameter_xml->QueryIntAttribute("ignoreProgramChange", &ignorePc);
		p.ignoreProgramChange = ignorePc != 0;

		double pminimum = 0.0;
		double pmaximum = 10.0;

		parameter_xml->QueryDoubleAttribute("RangeMinimum", &pminimum);
		parameter_xml->QueryDoubleAttribute("RangeMaximum", &pmaximum);

		int moduleHandle_ = -1;
		int moduleParamId_ = 0;
		bool isPolyphonic_ = false;
		std::wstring enumList_;

		parameter_xml->QueryIntAttribute("Module", &(moduleHandle_));
		parameter_xml->QueryIntAttribute("ModuleParamId", &(moduleParamId_));
		parameter_xml->QueryBoolAttribute("isPolyphonic", &(isPolyphonic_));

		if (p.dataType == gmpi::PinDatatype::Int32 || p.dataType == gmpi::PinDatatype::String /*|| dataType == DT_ENUM */)
		{
			auto s = parameter_xml->Attribute("MetaData");
			if (s)
				p.meta = s; // convert.from_bytes(s);
		}

		if (Private == 0)
		{
			assert(ParameterTag >= 0);
			//			p = makeNativeParameter(ParameterTag, pminimum > pmaximum);
			//			++nativeParameterCount;
			nativeParameters.push_back(std::make_unique<nativeParameter>(0)); // TODO index
		}
		else
		{
			//			p.isPolyphonic_ = isPolyphonic_;
		}

		//		p.hostControl_ = hostControl;
		p.minimum = pminimum;
		p.maximum = pmaximum;

#if 0
		parameter_xml->QueryIntAttribute("MIDI", &(p.MidiAutomation));
		if (p.MidiAutomation != -1)
		{
			std::string temp;
			parameter_xml->QueryStringAttribute("MIDI_SYSEX", &temp);
			p.MidiAutomationSysex = Utf8ToWstring(temp);
		}
#endif
#if 1
		// Default values from patch list.
		const auto dataType = p.dataType;
		ParseXmlPreset(
			parameter_xml,
			[p, dataType](int /*voiceId*/, int /*preset*/, const char* xmlvalue) mutable
			{
				p.defaultRaw.push_back(ParseToRaw((int)dataType, xmlvalue));
			}
		);

		// no patch-list?, init to zero.
		if (!parameter_xml->FirstChildElement("patch-list"))
		{
			assert(!stateful_);

			// Special case HC_VOICE_PITCH needs to be initialized to standard western scale
			if (HC_VOICE_PITCH == hostControl)
			{
				const int middleA = 69;
				constexpr float invNotesPerOctave = 1.0f / 12.0f;
				p.defaultRaw.reserve(128);
				for (float key = 0; key < 128; ++key)
				{
					const float pitch = 5.0f + static_cast<float>(key - middleA) * invNotesPerOctave;
					std::string raw((const char*)&pitch, sizeof(pitch));
					p.defaultRaw.push_back(raw);
				}
			}
			else
			{
				// init to zero
				const char* nothing = "\0\0\0\0\0\0\0\0";
				std::string raw(nothing, getDataTypeSize((int) dataType));
				p.defaultRaw.push_back(raw);
			}
		}
#endif

#if 0
		p.parameterHandle_ = ParameterHandle;
		p.datatype_ = dataType;
		p.moduleHandle_ = moduleHandle_;
		p.moduleParamId_ = moduleParamId_;
		p.stateful_ = stateful_;
		p.name_ = convert.from_bytes(Name);
		p.enumList_ = enumList_;
		p.ignorePc_ = ignorePc != 0;

		parameters_.push_back(std::unique_ptr<MpParameter>(p));
#endif
		//		ParameterHandleIndex.insert(std::make_pair(ParameterHandle, p));
		//		moduleParameterIndex.insert(std::make_pair(std::make_pair(moduleHandle_, moduleParamId_), ParameterHandle));

				// Ensure host queries return correct value.
		//		p.upDateImmediateValue();
	}
}
