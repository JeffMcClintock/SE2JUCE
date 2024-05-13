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

#ifdef _DEBUG
	std::string name;
#endif
};

struct paramValue
{
	//	std::vector< std::string > rawValues_; // rawValues_[voice] where voice is 0 - 127
	std::vector< RawData > rawValues_; // rawValues_[voice] where voice is 0 - 127

	gmpi::PinDatatype dataType = gmpi::PinDatatype::Float32;
};

struct DawPreset
{
	DawPreset(const std::map<int32_t, paramInfo>& parameters, std::string presetString, int presetIdx = 0);
	DawPreset(const std::map<int32_t, paramInfo>& parameters, class TiXmlNode* presetXml, int presetIdx = 0);
	DawPreset(const DawPreset& other);
	DawPreset() {}

	std::string toString(int32_t pluginId, std::string presetNameOverride = {}) const;
	void calcHash();
	bool empty() const
	{
		return hash == 0 && params.empty();
	}
	std::string name;
	std::string category;
	std::map<int32_t, paramValue> params;
	std::size_t hash = 0;
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

	void setPresetFromXml(const std::string& presetString);
	void setPresetFromUnownedPtr(DawPreset const* preset);

	void enableIgnoreProgramChange()
	{
		ignoreProgramChange = true;
	}
};

// additional support for retrieving the preset from the processor in a thread-safe manner.
class ProcessorStateMgrVst3 : public ProcessorStateMgr, public TimerClient
{
	DawPreset presetMutable;
	lock_free_fifo messageQue; // from real-time thread
	bool presetDirty = true;
	std::atomic<DawPreset const*> currentPreset;
//	std::unordered_map<int32_t, int32_t> tagToHandle;

	bool OnTimer() override;
	void serviceQueue();

protected:
	void setPreset(DawPreset const* preset) override;

public:
	ProcessorStateMgrVst3();

	void init(class TiXmlElement* parameters_xml) override;

	// Processor informing me of self-initiated parameter changes
	// from the real-time thread
	void SetParameterRaw(int32_t handle, RawView rawValue, int voiceId);

	DawPreset const* getPreset();
};
