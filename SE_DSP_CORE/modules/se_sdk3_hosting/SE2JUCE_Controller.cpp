#include "BinaryData.h"
#include "SE2JUCE_Controller.h"
#include "SE2JUCE_Processor.h"
#include "tinyxml/tinyxml.h"

MpParameterJuce::MpParameterJuce(SeJuceController* controller, int ParameterIndex, bool isInverted) :
	MpParameter_native(controller)
	,juceController(controller)
	,isInverted_(isInverted)
	,hostTag(ParameterIndex)
{
}

// After we recieve a preset, need to update native params value to DAW ASAP
// Note there is not guarantee what thread this happens on.
// Note that the parameter is not updated fully yet, that happens asyncronously on UI thread.
void MpParameterJuce::updateDawUnsafe(const std::string& rawValue)
{
	const RawView raw(rawValue.data(), rawValue.size());

	double normalized{};

	switch(datatype_)
	{
	default:
	{
		assert(false);
		return;
	}

	break;
	case DT_BOOL:
	{
		const auto value = (bool)raw;
		normalized = RealToNormalized(static_cast<double>(value));
	}
	break;
	case DT_INT:
	{
		const auto value = (int32_t)raw;
		normalized = RealToNormalized(static_cast<double>(value));
	}
	break;
	case DT_INT64:
	{
		const auto value = (int64_t)raw;
		normalized = RealToNormalized(static_cast<double>(value));
	}
	break;
	case DT_FLOAT:
	{
		const auto value = (float)raw;
		normalized = RealToNormalized(static_cast<double>(value));
	}
	break;
	case DT_DOUBLE:
	{
		const auto value = (double)raw;
		normalized = RealToNormalized(value);
	}
	break;
	}

//	auto juceParameter = juceController->processor->getParameters()[hostTag];
	if (juceParameter)
	{
		const bool handleGrabMyself = !isGrabbed();
		if (handleGrabMyself)
		{
			juceParameter->beginChangeGesture();
		}

		juceParameter->setValueNotifyingHost(adjust(static_cast<float>(normalized))); // param->getDawNormalized());

		if (handleGrabMyself)
		{
			juceParameter->endChangeGesture();
		}
	}
}


SeJuceController::SeJuceController(ProcessorStateMgrVst3& dawState) :
	queueToDsp_(SeAudioMaster::AUDIO_MESSAGE_QUE_SIZE)
	, dawStateManager(dawState)
{
}

void SeJuceController::setPresetXmlFromSelf(const std::string& xml)
{
	dawStateManager.setPresetFromXml(xml);
}

void SeJuceController::setPresetFromSelf(DawPreset const* preset)
{
	dawStateManager.setPresetFromUnownedPtr(preset);
}

void SeJuceController::setPresetUnsafe(DawPreset const* preset)
{
	interrupt_preset_.store(preset, std::memory_order_release);

	// TODO: Update Immediate values here right in callers stack frame, otherwise DAW might query a stale value

	const int voice = 0;
	for (const auto& [handle, val] : preset->params)
	{
		assert(handle != -1);

		auto it = ParameterHandleIndex.find(handle);
		if (it == ParameterHandleIndex.end())
			continue;

		auto& parameter = (*it).second;

		if (parameter->ignorePc_ && preset->ignoreProgramChangeActive && !preset->isInitPreset)
			continue;

		assert(parameter->datatype_ == (int)val.dataType);

		if (parameter->datatype_ != (int)val.dataType)
			continue;

		const auto& raw = val.rawValues_[voice];

		const std::string rawString((const char*) raw.data(), raw.size());
		parameter->updateDawUnsafe(rawString);
	}
}

std::string SeJuceController::getFactoryPresetXml(std::string filename)
{
	for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
	{
		if (filename == BinaryData::originalFilenames[i])
		{
			int dataSizeInBytes = {};
			const auto data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], dataSizeInBytes);
			return std::string(data, dataSizeInBytes);
		}
	}

	return {};
}

std::vector< presetInfo > SeJuceController::scanFactoryPresets()
{
	const char* xmlPresetExt = ".xmlpreset";

	std::vector< presetInfo > returnValues;

	for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
	{
		const std::string filename{ BinaryData::originalFilenames[i] };
		if (filename.find(xmlPresetExt) != std::string::npos)
		{
			const std::wstring fullpath = /*L"BinaryData/" +*/ ToWstring(filename);

			int dataSizeInBytes = {};
			const auto data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], dataSizeInBytes);
			const std::string xml(data, dataSizeInBytes);

			returnValues.push_back(parsePreset(fullpath, xml));
		}
	}

	return returnValues;
}

#if 0
void SeJuceController::loadFactoryPreset(int index, bool fromDaw)
{
	const char* xmlPresetExt = ".xmlpreset";
	int presetIndex = 0;
	for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
	{
		const std::string filename{ BinaryData::originalFilenames[i] };
		if (filename.find(xmlPresetExt) != std::string::npos && presetIndex++ == index)
		{
			int dataSizeInBytes = {};
			const auto data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], dataSizeInBytes);
			const std::string xml(data, dataSizeInBytes);

#if 1
			dawStateManager.setPresetFromXml(xml);
#else
			if (fromDaw)
			{
				setPresetFromDaw(xml, true);
			}
			else // from internal preset browser
			{
				TiXmlDocument doc;
				doc.Parse(xml.c_str());

				if (doc.Error())
				{
					assert(false);
					return;
				}

				TiXmlHandle hDoc(&doc);

				setPreset(hDoc.ToNode(), true, presetIndex);
			}
#endif

			return;
		}
	}
}
#endif

// Mode should remain on 'Master' unless explicitly set by user.
void SeJuceController::OnStartupTimerExpired()
{
	// disable updates to 'Master/Analyse' via loading presets.
	MpController::OnStartupTimerExpired();

	dawStateManager.enableIgnoreProgramChange();
}

void MpParameterJuce::setNormalizedUnsafe(float daw_normalized)
{
	dawNormalizedUnsafe = daw_normalized;

	dirty.store(true, std::memory_order_release);
	juceController->setDirty();
}

// this is the DAW setting a parameter.
// it needs to be relayed to the Controller (and GUI) and to the Processor
void MpParameterJuce::updateFromImmediate()
{
	if (const auto pdirty = dirty.exchange(false, std::memory_order_relaxed); !pdirty)
	{
		return;
	}

	const float se_normalized = adjust(dawNormalizedUnsafe.load(std::memory_order_relaxed));

	// this will update all GUIs
	if (setParameterRaw(gmpi::MP_FT_NORMALIZED, sizeof(se_normalized), &se_normalized))
	{
/* now done in parallel on processor
		// this will update the processor only, not the DAW.
		assert(juceController == controller_);
		controller_->ParamToDsp(this);
*/
	}
}

void MpParameterJuce::updateProcessor(gmpi::FieldType fieldId, int32_t voice)
{
	if(fieldId == gmpi::MP_FT_VALUE || fieldId == gmpi::MP_FT_NORMALIZED)
		juceController->ParamToProcessorAndHost(this);
}

void SeJuceController::ParamGrabbed(MpParameter_native* param)
{
	auto juceParameter = processor->getParameters()[param->getNativeTag()];
	if (juceParameter)
	{
		if (param->isGrabbed())
			juceParameter->beginChangeGesture();
		else
			juceParameter->endChangeGesture();
	}
}

void SeJuceController::ParamToProcessorAndHost(MpParameterJuce* param)
{
    // JUCE does not provide for thread-safe notification to the processor, so handle this via the message queue.

    // NOTE: juceParameter->setValueNotifyingHost() also updates the DSP via the timer, but *only* if it detects a change in the value (which is often won't)
    ParamToDsp(param);

    // update JUCE parameter
    auto juceParameter = processor->getParameters()[param->getNativeTag()];
    if (juceParameter)
    {
        const bool handleGrabMyself = !param->isGrabbed();
        if (handleGrabMyself)
        {
            juceParameter->beginChangeGesture();
        }

        juceParameter->setValueNotifyingHost(param->getDawNormalized());

        if (handleGrabMyself)
        {
            juceParameter->endChangeGesture();
        }
    }
}

void SeJuceController::ParamToDsp(MpParameter* param, int32_t voiceId)
{
	MpController::ParamToDsp(param, voiceId);

	if (param->stateful_)
	{
		const auto field = gmpi::MP_FT_VALUE;
		const auto rawValue = param->getValueRaw(field, voiceId);
		const int32_t messageSize = 4 * sizeof(int32_t) + static_cast<int32_t>(rawValue.size());

		auto& queue = *ControllerToStateMgrQue();

		if (!my_msg_que_output_stream::hasSpaceForMessage(&queue, messageSize))
		{
			// queue full. drop message.
			// _RPTN(0, "ControllerToStateMgrQue: QUEUE FULL!!! (%d bytes message)\n", size);
			assert(false);
			return;
		}

		my_msg_que_output_stream strm(&queue, param->parameterHandle_, "ppc");
		strm << messageSize;
		strm << voiceId;
		strm << field;
		strm << static_cast<int32_t>(rawValue.size());
		strm.Write(
			rawValue.data(),
			static_cast<int32_t>(rawValue.size())
		);

		strm.Send();
	}
}

// ensure GUI reflects the value of all parameters
void SeJuceController::initGuiParameters()
{
	const int voice = 0;
	for (auto& p : parameters_)
	{
		updateGuis(p.get(), voice);
	}
}

void SeJuceController::OnLatencyChanged()
{
	processor->OnLatencyChanged();
}
