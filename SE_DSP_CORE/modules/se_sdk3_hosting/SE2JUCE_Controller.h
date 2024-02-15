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

	float adjust(float normalised) const
	{
		return isInverted_ ? 1.0f - normalised : normalised;
	}

public:

	MpParameterJuce(class SeJuceController* controller, int ParameterIndex, bool isInverted);

	int getNativeTag() override { return hostTag; }

	void setNormalizedUnsafe(float daw_normalized);

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

	// not required for JUCE.
	void upDateImmediateValue() override{}
};


class SeJuceController : public MpController, public IHasDirty, private juce::Timer, public IProcessorMessageQues
{
	std::atomic<bool> juceParameters_dirty;
	std::vector<MpParameterJuce* > tagToParameter;			// DAW parameter Index to parameter
	class SE2JUCE_Processor* processor = {};
    interThreadQue queueToDsp_;

public:
	SeJuceController();

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

	void ParamGrabbed(MpParameter_native* param, int32_t voice = 0) override
	{}
	
//	void legacySendProgramChangeToHost(float normalised) override;

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

	void ParamToProcessorAndHost(MpParameterJuce* param, gmpi::FieldType fieldId, int32_t voice);

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
