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
// Note there is not gaurentee what thread this happens on.
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

		juceParameter->setValueNotifyingHost(adjust(normalized)); // param->getDawNormalized());

		if (handleGrabMyself)
		{
			juceParameter->endChangeGesture();
		}
	}
}


SeJuceController::SeJuceController(DawStateManager& dawState) :
	MpController(dawState)
	,queueToDsp_(SeAudioMaster::AUDIO_MESSAGE_QUE_SIZE)
{
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

std::vector< MpController::presetInfo > SeJuceController::scanFactoryPresets()
{
	const char* xmlPresetExt = ".xmlpreset";

	std::vector< MpController::presetInfo > returnValues;

	for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
	{
		const std::string filename{ BinaryData::originalFilenames[i] };
		if (filename.find(xmlPresetExt) != std::string::npos)
		{
			const std::wstring fullpath = L"BinaryData/" + ToWstring(filename);

			int dataSizeInBytes = {};
			const auto data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], dataSizeInBytes);
			const std::string xml(data, dataSizeInBytes);

			returnValues.push_back(parsePreset(fullpath, xml));
		}
	}

	return returnValues;
}

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
			dawStateManager.setPreset(xml);
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

// Mode should remain on 'Master' unless explicity set by user.
void SeJuceController::OnStartupTimerExpired()
{
	// disable updates to 'Master/Analyse' via loading presets.
	MpController::OnStartupTimerExpired();

//??	undoManager.initial(this);

#ifdef SE_USE_DAW_STATE_MGR
	dawStateManager.enableIgnoreProgramChange();
#endif
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
	const auto pdirty = dirty.exchange(false, std::memory_order_relaxed);
	if (!pdirty)
	{
		return;
	}

	const float se_normalized = adjust(dawNormalizedUnsafe.load(std::memory_order_relaxed));

	// this will update all GUIs
	if (setParameterRaw(gmpi::MP_FT_NORMALIZED, sizeof(se_normalized), &se_normalized))
	{
		//updateProcessor(gmpi::MP_FT_VALUE, 0);

		// this will update the processor only, not the DAW.
		//juceController->ParamToDsp(this);
		assert(juceController == controller_);
		controller_->ParamToDsp(this);
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
	// also need to notify JUCE and DAW.
#if 0
	switch (fieldId)
	{
	case gmpi::MP_FT_GRAB:
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
		break;
	case gmpi::MP_FT_VALUE:
	case gmpi::MP_FT_NORMALIZED:
		{
#endif
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
	//	}
	//	break;
	//}
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
