#include "./ProcessorStateManager.h"
#include "conversion.h"
#include "RawConversions.h"
#include "modules/shared/xp_simd.h"
#include "PresetReader.h"
#include "SeAudioMaster.h"
#include "my_msg_que_input_stream.h"
#include "tinyxml/tinyxml.h"
#include "mfc_emulation.h"

std::string normalizedToRaw(int32_t datatype, float fnormalized, double maximum, double minimum, const std::wstring& enumList)
{
	const double normalized = static_cast<double>(fnormalized);

	// Update real world normalized in 'rawValue_'.
	double realWorld = 0.0;

	// Private parameter.
	if (enumList.empty()) // float, double, int.
	{
		realWorld = minimum + normalized * (maximum - minimum);
	}
	else // enum.
	{
		realWorld = (double)normalised_to_enum(enumList, static_cast<float>(normalized));
	}

	std::string newRawValue;

	switch (datatype)
	{
	case DT_FLOAT:
	{
		newRawValue = ToRaw4((float)realWorld);
		break;
	}
	case DT_DOUBLE:
	{
		newRawValue = ToRaw4(realWorld);
		break;
	}
	case DT_INT:
	{
		// -ves fail			newRawValue = ToRaw4((int32_t)(0.5 + realWorld));
		newRawValue = ToRaw4((int32_t)FastRealToIntFloor(0.5 + realWorld));
		break;
	}
	case DT_INT64:
	{
		// -ves fail			newRawValue = ToRaw4((int64_t)(0.5 + realWorld));
		newRawValue = ToRaw4((int64_t)FastRealToIntFloor(0.5 + realWorld));
		break;
	}
	case DT_BOOL:
	{
		newRawValue = ToRaw4((bool)(normalized > 0.5));
		break;
	}

	default:
		assert(false);
		break;
	}

	return newRawValue;
}

#if 0 // seems redundant provided param update ALWAYS get through to patch-manager

// notification of parameter automation from the DAW on the real-time thread.
// this will allow us to catch parameter updates while the processor is restating otherwise they get dropped.
// TODO. is it possible to suppress duplicate updates from the dsp patch manager?
void ProcessorStateMgr::setParameter(int32_t tag, float normalized)
{
	//	normalizedValues[tag]->store(normalized, std::memory_order_relaxed);

	const int32_t messageSize = sizeof(int32_t) + sizeof(float);

	my_msg_que_output_stream strm(&messageQue, tag, "norm");
	strm << messageSize;
	strm << normalized;

	strm.Send();
}
#endif

DawPreset::DawPreset(const DawPreset& other)
{
	name = other.name;
	hash = other.hash;
	params = other.params;
	category = other.category;
	resetUndo = other.resetUndo;
	ignoreProgramChangeActive = other.ignoreProgramChangeActive;
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
			values.rawValues_.push_back(ParseToRaw((int)info.dataType, v));

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

			if (info.ignoreProgramChange)
				continue;

			auto& values = params[paramHandle];
			values.dataType = info.dataType;

			// Preset values from patch list.
			ParseXmlPreset(
				ParamElement,
				[values, info](int /*voiceId*/, int /*preset*/, const char* xmlvalue) mutable
				{
					values.rawValues_.push_back(ParseToRaw((int)info.dataType, xmlvalue));
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
	// except for preset name and category
	for (auto& [paramHandle, info] : parametersInfo)
	{
		if (info.hostControl == HC_PROGRAM_NAME || HC_PROGRAM_CATEGORY == info.hostControl)
			continue;

		if (params.find(paramHandle) == params.end())
		{
			auto& values = params[paramHandle];

			values.dataType = info.dataType;
			for (const auto& v : info.defaultRaw)
			{
				values.rawValues_.push_back(v);
			}
		}
	}

#ifdef _DEBUG
	for (auto& p : params)
	{
		assert(p.second.rawValues_.size() == 1);
	}
#endif

	calcHash();
}

std::string DawPreset::toString(int32_t pluginId, std::string presetNameOverride) const
{
	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "", "");
	doc.LinkEndChild(decl);

	auto element = new TiXmlElement("Preset");
	doc.LinkEndChild(element);

	{
		char txt[20];
#if defined(_MSC_VER)
		sprintf_s(txt, "%08x", pluginId);
#else
		snprintf(txt, std::size(txt), "%08x", pluginId);
#endif
		element->SetAttribute("pluginId", txt);
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

		const auto val = RawToUtf8B((int)parameter.dataType, raw.data(), raw.size());

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

void DawPreset::calcHash()
{
	const auto xml = toString(0); // using blank (0) plugin-id to give more consistent hash if user changes plugin code in future.
	hash = std::hash<std::string>{}(xml);

#if 0 //def _DEBUG
	{
		auto xmls = xml.substr(0, 500);
		_RPTN(0, "DawPreset::calcHash Preset: %s hash %4x\n %s\n\n", name.c_str(), hash, xmls.c_str());
	}
#endif
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessorStateMgr::setPresetFromUnownedPtr(DawPreset const* preset)
{
	setPreset(retainPreset(new DawPreset(*preset)));
}

ProcessorStateMgrVst3::ProcessorStateMgrVst3() : messageQue(SeAudioMaster::AUDIO_MESSAGE_QUE_SIZE)
{
	presetMutable.name = "Init";
}

// parameter changed from any source in real-time thread (GUI/DAW/Meters)
// push change onto queue.
// The purpose is to allow the state manager to provide the preset to the DAW at any time/thread.
void ProcessorStateMgrVst3::SetParameterRaw(int32_t paramHandle, RawView rawValue, int32_t voiceId)
{
	// ignore non-stateful params
	auto it = parametersInfo.find(paramHandle);
	if (it == parametersInfo.end())
		return;

	const int32_t messageSize = 2 * sizeof(int32_t) + static_cast<int32_t>(rawValue.size());

	my_msg_que_output_stream strm(&messageQue, paramHandle, "ppc");
	strm << messageSize;
	strm << voiceId;
	strm << static_cast<int32_t>(rawValue.size());
	strm.Write(
		rawValue.data(),
		static_cast<int32_t>(rawValue.size())
	);

	strm.Send();
}

#if 0
ProcessorStateMgrAu2::ProcessorStateMgrAu2() : messageQue(SeAudioMaster::AUDIO_MESSAGE_QUE_SIZE)
{
    
}

// communicate parameter automation to the GUI (since Apple DAWs don't bother using the parameter listener functionality).
// value is real value, not normalised.
void ProcessorStateMgrAu2::onParameterAutomation(int32_t paramTag, float value)
{
	const int32_t messageSize = sizeof(value);

	my_msg_que_output_stream strm(&messageQue, paramTag, "norm");
	strm << messageSize;
	strm << value;

	strm.Send();
}

// GUI to call this on a timer to get any parameter automation updates.
void ProcessorStateMgrAu2::queryUpdates(IAuGui* ui)
{
	while (messageQue.readyBytes() > 0)
	{
		int32_t paramTag;
		int32_t recievingMessageId;
		int32_t recievingMessageLength;

		my_msg_que_input_stream strm(&messageQue);
		strm >> paramTag;
		strm >> recievingMessageId;
		strm >> recievingMessageLength;

		assert(recievingMessageId != 0);
		assert(recievingMessageLength >= 0);

		switch (recievingMessageId)
		{
		case id_to_long2("norm"):
		{
			float value;
			strm >> value;

			ui->OnParameterUpdateFromDaw(paramTag, value);
		}
		break;

		default:
			assert(false);
		};
	}
}
#endif

// only for debugging
std::string RawToXml(gmpi::PinDatatype dataType, RawView rawVal)
{
	switch (dataType)
	{
	case gmpi::PinDatatype::Float32:
	{
		const auto value = RawToValue<float>(rawVal.data(), rawVal.size());
		return MyTypeTraits<float>::toXML(value);
	}
	break;
	case gmpi::PinDatatype::Float64:
	{
		const auto value = RawToValue<double>(rawVal.data(), rawVal.size());
		return MyTypeTraits<double>::toXML(value);
	}
	break;
	case gmpi::PinDatatype::Int32:
	{
		const auto value = RawToValue<int32_t>(rawVal.data(), rawVal.size());
		return MyTypeTraits<int32_t>::toXML(value);
	}
	break;
	case gmpi::PinDatatype::Int64:
	{
		const auto value = RawToValue<int64_t>(rawVal.data(), rawVal.size());
		return MyTypeTraits<int64_t>::toXML(value);
	}
	break;
	case gmpi::PinDatatype::Bool:
	{
		const auto value = RawToValue<bool>(rawVal.data(), rawVal.size());
		return MyTypeTraits<bool>::toXML(value);
	}
	break;

	case gmpi::PinDatatype::String:
	{
		const auto value = RawToValue<std::wstring>(rawVal.data(), rawVal.size());
		return JmUnicodeConversions::WStringToUtf8(value);
	}
	break;

	case gmpi::PinDatatype::Enum:
	{
		return "<enum>";
	}
	break;

	case gmpi::PinDatatype::Blob:
	{
		return "<BLOB>";
	}
	break;

	default:
		assert(false);
		break;
	}

	return {};
}

// service Que
bool ProcessorStateMgrVst3::OnTimer()
{
	if (messageQue.readyBytes() > 0)
	{
		const std::lock_guard<std::mutex> lock{ presetMutex };
		presetDirty = true;

		serviceQueue();
	}

	return true;
}

// must be called under presetMutex lock.
void ProcessorStateMgrVst3::serviceQueue()
{
	while (messageQue.readyBytes() > 0)
	{
		int32_t paramHandle;
		int32_t recievingMessageId;
		int32_t recievingMessageLength;

		my_msg_que_input_stream strm(&messageQue);
		strm >> paramHandle;
		strm >> recievingMessageId;
		strm >> recievingMessageLength;

		assert(recievingMessageId != 0);
		assert(recievingMessageLength >= 0);

		switch (recievingMessageId)
		{
#if 0 // seems redundant provided param update ALWAYS get through to patch-manager
		case id_to_long2("norm"):
		{
			float normalized;
			strm >> normalized;

			const auto it = tagToHandle.find(paramHandle);
			if (it == tagToHandle.end())
				continue;

			const auto paramHandle = it->second;
			const auto it2 = parametersInfo.find(paramHandle);
			if (it2 == parametersInfo.end())
				continue;

			const auto& info = it2->second;

			// convert normalized to actual value.
			auto& rawVal = presetMutable.params[paramHandle].rawValues_[0];

			rawVal
				= normalizedToRaw(
					(int32_t)info.dataType
					, normalized
					, info.maximum
					, info.minimum
					, info.meta
				);

#ifdef DEBUG
			const auto asString = RawToXml(info.dataType, rawVal);
			_RPTN(0, "PSM: param:%d norm:%f val:%s\n", paramHandle, normalized, asString.c_str());
#endif
			// update normalized value
			//normalizedValues[paramHandle]->store(normalized, std::memory_order_relaxed);
		}
		break;
#endif

		case id_to_long2("ppc"):
		{
			int32_t voiceId;
			std::string rawValue;

			strm >> voiceId;
			strm >> rawValue;

			const auto it = parametersInfo.find(paramHandle);
			if (it == parametersInfo.end())
				continue;

			const auto& info = it->second;

#if 0 //def DEBUG
			const auto asString = RawToXml(info.dataType, rawValue);
			_RPTN(0, "PSM: param:%d val:%s\n", paramHandle, asString.c_str());
#endif
			if (info.hostControl != HC_PROGRAM_NAME && HC_PROGRAM_CATEGORY != info.hostControl)
			{
#if 0
				// convert raw to normalized
				const auto normalized = rawToNormalized(
					(int32_t)info.dataType
					, rawValue
					, info.maximum
					, info.minimum
					, info.meta
				);

				// update normalized value
				normalizedValues[info.tag]->store(normalized, std::memory_order_relaxed);
#endif
				// update preset
				assert(presetMutable.params.find(paramHandle) != presetMutable.params.end());
				assert(!presetMutable.params[paramHandle].rawValues_.empty());

				presetMutable.params[paramHandle].rawValues_[0] = rawValue;
			}
			else
			{
//				_RPT0(0, "PSM: IGNORED program name/category\n");
			}
		}
		break;

		default:
			assert(false);
		};
	}
}

void ProcessorStateMgr::setPresetFromXml(const std::string& presetString)
{
	auto preset = new DawPreset(parametersInfo, presetString);
	setPreset(retainPreset(preset));
}

void ProcessorStateMgrVst3::setPreset(DawPreset const* preset)
{
	{
		const std::lock_guard<std::mutex> lock{ presetMutex };
		serviceQueue(); // empty the FIFO otherwise it might apply stale changes to the new preset.
		presetMutable = *preset;

		currentPreset.store(preset, std::memory_order_release);
	}

	ProcessorStateMgr::setPreset(preset);
}

void ProcessorStateMgr::setPreset(DawPreset const* preset)
{
	preset->ignoreProgramChangeActive = ignoreProgramChange;
	callback(preset);
}

DawPreset const* ProcessorStateMgrVst3::getPreset()
{
	// bring current preset up-to-date
	OnTimer();

	if (presetDirty)
	{
		DawPreset* preset{};
		// make a copy of the mutable preset
		{
			const std::lock_guard<std::mutex> lock{ presetMutex };
			preset = new DawPreset(presetMutable);
			presetDirty = false;
		}
#if 0
		// apply latest values from DAW
		// not synced in anyway, so might get sliced as we read them out.
		for (auto& pn : normalizedValues)
		{
			const auto paramHandle = tagToHandle[pn.first];

			assert(preset->params.find(paramHandle) != preset->params.end());
			assert(parametersInfo.find(paramHandle) != parametersInfo.end());

			const auto& info = parametersInfo[paramHandle];

			const auto normalized = pn.second->load(std::memory_order_relaxed);

			// convert nomalised to actual value.
			preset->params[paramHandle].rawValues_[0]
				= normalizedToRaw(
					(int32_t)info.dataType
					, normalized
					, info.maximum
					, info.minimum
					, info.meta
				);
		}
#endif
		retainPreset(preset);
		currentPreset.store(preset, std::memory_order_release);
	}

	assert(currentPreset.load(std::memory_order_relaxed) != nullptr);

	return currentPreset.load(std::memory_order_relaxed);
}

DawPreset const* ProcessorStateMgr::retainPreset(DawPreset const* preset)
{
	// retain immutable preset
	const std::lock_guard<std::mutex> lock{ presetMutex };

	presets.push_back(std::unique_ptr<const DawPreset>(preset));

	return preset;
}

void ProcessorStateMgr::init(TiXmlElement* parameters_xml)
{
	const std::lock_guard<std::mutex> lock{ presetMutex };

	::init(parametersInfo, parameters_xml);
}

void init(std::map<int32_t, paramInfo>& parametersInfo, class TiXmlElement* parameters_xml)
{
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

		parametersInfo[ParameterHandle] = {};
		paramInfo& p = parametersInfo[ParameterHandle];


#ifdef _DEBUG
		p.name = parameter_xml->Attribute("Name");
#endif
		{
			int temp{ (int)gmpi::PinDatatype::Float32 };
			parameter_xml->QueryIntAttribute("ValueType", &temp);
			p.dataType = static_cast<gmpi::PinDatatype>(temp);

			// wheras pin enum values are 16-bit, enum parameters use 32-bit ints. otherwise ParseToRaw returns only 2-bytes
			if (p.dataType == gmpi::PinDatatype::Enum)
			{
				// VST and AU can't handle this type of parameter.
				p.dataType = gmpi::PinDatatype::Int32;
			}
		}

		parameter_xml->QueryIntAttribute("Index", &p.tag);
		int Private = 0;
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

		parameter_xml->QueryIntAttribute("HostControl", &p.hostControl);
		int ignorePc = 0;
		parameter_xml->QueryIntAttribute("ignoreProgramChange", &ignorePc);
		p.ignoreProgramChange = ignorePc != 0;

		parameter_xml->QueryDoubleAttribute("RangeMinimum", &p.minimum);
		parameter_xml->QueryDoubleAttribute("RangeMaximum", &p.maximum);

		if (p.dataType == gmpi::PinDatatype::Int32 || p.dataType == gmpi::PinDatatype::String /*|| dataType == DT_ENUM */)
		{
			if (auto s = parameter_xml->Attribute("MetaData"); s)
				p.meta = JmUnicodeConversions::Utf8ToWstring(s);
		}

		//if (Private == 0 && p.tag != -1)
		//{
		//	//			normalizedValues[p.tag] = std::make_unique<std::atomic<float>>(0.0f);
		//	tagToHandle[p.tag] = ParameterHandle;
		//}

#if 0
		parameter_xml->QueryIntAttribute("MIDI", &(p.MidiAutomation));
		if (p.MidiAutomation != -1)
		{
			std::string temp;
			parameter_xml->QueryStringAttribute("MIDI_SYSEX", &temp);
			p.MidiAutomationSysex = Utf8ToWstring(temp);
		}
#endif
		// Default values from patch list.
		const auto dataType = p.dataType;
		ParseXmlPreset(
			parameter_xml,
			[&p, dataType](int /*voiceId*/, int /*preset*/, const char* xmlvalue) mutable
			{
				p.defaultRaw.push_back(ParseToRaw((int)dataType, xmlvalue));
			}
		);

		// no patch-list?, init to zero.
		if (p.defaultRaw.empty()) //!parameter_xml->FirstChildElement("patch-list"))
		{
			assert(!stateful_);

			// Special case HC_VOICE_PITCH needs to be initialized to standard western scale
			if (HC_VOICE_PITCH == p.hostControl)
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
				std::string raw(nothing, getDataTypeSize((int)dataType));
				p.defaultRaw.push_back(raw);
			}
		}
	}

#ifdef _DEBUG
	for (auto& p : parametersInfo)
	{
		assert(!p.second.defaultRaw.empty());
	}
#endif
}

void ProcessorStateMgrVst3::init(TiXmlElement* parameters_xml)
{
	ProcessorStateMgr::init(parameters_xml);

	// init the current preset
	for (auto& p : parametersInfo)
	{
		auto& presetMutableParam = presetMutable.params[p.first];
		presetMutableParam.dataType = p.second.dataType;
		for (auto& v : p.second.defaultRaw)
			presetMutableParam.rawValues_.push_back({ v.data(), v.size() });
	}

	StartTimer(177);
}
