#pragma once
#include "JuceHeader.h"
#include <atomic>
#include <vector>
#include "Controller.h"
#include "IProcessorMessageQues.h"

struct IHasDirty
{
	virtual void setDirty() = 0;
};

// !!! juce::AsyncUpdater might be helpful here
class MpParameterJuce : public MpParameter_native
{
	std::atomic<float> dawNormalizedUnsafe; // incoming changes from the DAW.
	std::atomic<bool> dirty;
	class SeJuceController* juceController = {};
	bool isInverted_ = false;
	int hostTag = -1;	// index, sequential.
	bool dawGrabbed = false; // the grabbed state we last sent to the DAW (logical OR of all grabbers)
	juce::AudioProcessorParameter* juceParameter = {};

	float adjust(float normalised) const
	{
		return isInverted_ ? 1.0f - normalised : normalised;
	}

public:

	MpParameterJuce(class SeJuceController* controller, int ParameterIndex, bool isInverted);
	void setJuceParameter(juce::AudioProcessorParameter* parameter)
	{
		juceParameter = parameter;
	}
	int getNativeTag() override { return hostTag; }

	void setNormalizedUnsafe(float daw_normalized);
	void updateDawUnsafe(const std::string& rawValue) override;

	// on the foreground thread, update the parameter from the unsafe value provided by the DAW
	void updateFromImmediate();

	float getDawNormalized() const
	{
		return adjust(getNormalized());
	}

	void setGrabbed(bool isGrabbed)
	{
		setParameterRaw(gmpi::MP_FT_GRAB, sizeof(isGrabbed), &isGrabbed);
	}

	void updateProcessor(gmpi::FieldType fieldId, int32_t voice) override;

	void onGrabbedChanged() override
	{
		if (isGrabbed() != dawGrabbed)
		{
			dawGrabbed = isGrabbed();

			controller_->ParamGrabbed(this);
		}
	}

	// not required for JUCE.
	void upDateImmediateValue() override{}
};


class SeJuceController : public MpController, public IHasDirty, private juce::Timer, public IProcessorMessageQues
{
	std::atomic<bool> juceParameters_dirty;
	std::vector<MpParameterJuce* > tagToParameter;			// DAW parameter Index to parameter
	class SE2JUCE_Processor* processor = {};
    interThreadQue queueToDsp_;
	std::atomic<DawPreset const*> interrupt_preset_ = {};
	DawStateManager& dawStateManager;

public:
	SeJuceController(
		DawStateManager& dawState
	);

	void Initialize(SE2JUCE_Processor* pprocessor) 
	{
		processor = pprocessor;

		undoManager.enabled = true;
		MpController::Initialize();

		ScanPresets(); // !! could be consolidated in MpController

		// SE Timer suffers under JUCE, becomes very unresponsive.
//		StartTimer(35); // SE. approx 30Hz
		startTimerHz(24); // JUCE. 
	}

	void OnStartupTimerExpired() override;

	const auto& nativeParameters() const
	{
		return tagToParameter;
	}

	void setPresetXmlFromSelf(const std::string& xml) override;
	void setPresetFromSelf(DawPreset const* preset) override;
	void setPresetUnsafe(DawPreset const* preset);
	std::string loadNativePreset(std::wstring sourceFilename) override
	{
		return {};
	}
	void saveNativePreset(const char* filename, const std::string& presetName, const std::string& xml) override
	{}
	std::wstring getNativePresetExtension() override
	{
		return L"xmlpreset";
	}
	std::vector< MpController::presetInfo > scanFactoryPresets() override;
	void loadFactoryPreset(int index, bool fromDaw) override;
	std::string getFactoryPresetXml(std::string filename) override;

	// IHasDirty
	void setDirty() override
	{
		juceParameters_dirty.store(true, std::memory_order_release);
	}

// SE	bool OnTimer() override
	void timerCallback() override // JUCE timer
	{
		if (juceParameters_dirty.load(std::memory_order_relaxed))
		{
			juceParameters_dirty.store(false, std::memory_order_release);

			for (auto p : tagToParameter)
			{
				p->updateFromImmediate();
			}
		}

		if (auto preset = interrupt_preset_.exchange(nullptr, std::memory_order_relaxed); preset)
		{
			setPreset(preset);
		}

		// SE return MpController::OnTimer();
		MpController::OnTimer();
	}

	MpParameterJuce* getDawParameter(int nativeTag)
	{
		if(nativeTag >= 0 && nativeTag < tagToParameter.size())
		{
			return tagToParameter[nativeTag];
		}

		return {};
	}

	void setParameterNormalizedUnsafe(int nativeTag, float newValue)
	{
		if (auto p = getDawParameter(nativeTag) ; p)
		{
			p->setNormalizedUnsafe(newValue);
		}
	}

	void ParamGrabbed(MpParameter_native* param) override;
	void ParamToProcessorAndHost(MpParameterJuce* param);

	MpParameter_native* makeNativeParameter(int, bool isInverted = false) override
	{
		// JUCE uses a strict sequential parameter index, not the more relaxed tags of VST3 etc
		const auto sequentialIndex = static_cast<int>(tagToParameter.size());
		tagToParameter.push_back(new MpParameterJuce(this, sequentialIndex, isInverted));
		return tagToParameter.back();
	}

//	void OnInitialPresetRecieved();

	void initGuiParameters();

	// IProcessorMessageQues
	IWriteableQue* MessageQueToGui()  override
	{
        return &message_que_dsp_to_ui;
	}
	void Service()  override{} // VST3 only.
	interThreadQue* ControllerToProcessorQue()  override
	{
		return &queueToDsp_;
	}
	IWriteableQue* getQueueToDsp() override
	{
		return &queueToDsp_;
	}
};
