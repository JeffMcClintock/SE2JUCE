#pragma once
#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include "TimerManager.h"
#include "lock_free_fifo.h"
#include "GmpiApiCommon.h"
#include "modules/shared/RawView.h"

/*
#include "ProcessorStateManager.h"
*/

struct paramInfo
{
	std::vector< std::string > defaultRaw;
	gmpi::PinDatatype dataType = gmpi::PinDatatype::Float32;
	double minimum = 0.0;
	double maximum = 10.0;
	std::wstring meta; // enum list/file ext
	bool private_ = false;
	bool ignoreProgramChange = false;
	int32_t hostControl = -1;
	int32_t tag = -1;

	int32_t midiAutomation = -1;
	std::wstring midiAutomationSysex;

#ifdef _DEBUG
	std::string name;
#endif
};

struct paramValue
{
	std::vector< RawData > rawValues_; // where voice is 0 - 127

	gmpi::PinDatatype dataType = gmpi::PinDatatype::Float32;
	int32_t MidiAutomation = -1;
	std::wstring MidiAutomationSysex;
};

struct DawPreset
{
	DawPreset(const std::map<int32_t, paramInfo>& parameters, std::string presetString, int presetIdx = 0);
	DawPreset(const std::map<int32_t, paramInfo>& parameters, class TiXmlNode* presetXml, int presetIdx = 0);
	DawPreset(const DawPreset& other);
	DawPreset() {}

	std::string toString(int32_t pluginId, std::string presetNameOverride = {}, int withVersion = -1) const;
	void calcHash();
	bool empty() const
	{
		return hash == 0 && params.empty();
	}
	std::string name;
	std::string category;
	std::map<int32_t, paramValue> params;
	std::size_t hash = 0;
	bool isInitPreset = false;
	mutable bool ignoreProgramChangeActive = false;
	mutable bool resetUndo = true;

private:
	void initFromXML(const std::map<int32_t, paramInfo>& parametersInfo, TiXmlNode* presetXml, int presetIdx);
};

void init(std::map<int32_t, paramInfo>& parametersInfo, class TiXmlElement* parameters_xml);

class ProcessorStateMgr
{
protected:
	std::map<int32_t, paramInfo> parametersInfo;
	std::vector< std::unique_ptr<const DawPreset> > presets;
	std::mutex presetMutex;
	bool ignoreProgramChange = false;

protected:
	DawPreset const* retainPreset(DawPreset const* preset);
	virtual void setPreset(DawPreset const* preset);

public:
	std::function<void(DawPreset const*)> callback;

	ProcessorStateMgr() {}
	virtual void init(class TiXmlElement* parameters_xml);

	virtual void setPresetFromXml(const std::string& presetString);
	void setPresetFromUnownedPtr(DawPreset const* preset);

	void enableIgnoreProgramChange()
	{
		ignoreProgramChange = true;
	}
	const std::map<int32_t, paramInfo>& getParametersInfo() const
	{
		return parametersInfo;
	}
};

// additional support for retrieving the preset from the processor in a thread-safe manner.
// Also used by SE2JUCE
class ProcessorStateMgrVst3 : public ProcessorStateMgr, public TimerClient
{
	DawPreset presetMutable;
	lock_free_fifo messageQueFromProcessor; // from real-time thread
	lock_free_fifo messageQueFromController; // message thread
	bool presetDirty = true;
	std::atomic<DawPreset const*> currentPreset;

	bool OnTimer() override;
	void serviceQueue(lock_free_fifo& fifo);

protected:
	void setPreset(DawPreset const* preset) override;
	void QueueParameterUpdate(lock_free_fifo* fifo, int32_t handle, int32_t field, RawView rawValue, int voiceId);

public:
	ProcessorStateMgrVst3();

	void init(class TiXmlElement* parameters_xml) override;
	void setPresetFromXml(const std::string& presetString) override;
	void setPresetRespectingIpc(DawPreset* preset);

	// Processor informing me of self-initiated parameter changes
	// from the real-time thread
	void SetParameterFromProcessor(int32_t handle, int32_t field, RawView rawValue, int voiceId);
	void ProcessorWatchdog();

	// message-thread
	InterThreadQueBase* ControllerToStateMgrQue()
	{
		return &messageQueFromController;
	}
	DawPreset const* getPreset();
};

#if 0
struct IAuGui
{
	virtual void OnParameterUpdateFromDaw(int32_t tag, float normalised) = 0;
};

// Additional support for passing parameter updates to the UI in a thread-safe manner.
class ProcessorStateMgrAu2 : public ProcessorStateMgr
{
	lock_free_fifo messageQue; // from real-time thread

public:
    ProcessorStateMgrAu2();
    
	void onParameterAutomation(int32_t dawParameterId, float value);
	void queryUpdates(IAuGui* ui);
//	void OnTimer();
//	void serviceQueue();
};
#endif
