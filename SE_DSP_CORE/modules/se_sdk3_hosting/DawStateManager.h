#pragma once

/*
#include "DawStateManager.h"
*/

#include <map>
#include <mutex>
#include <vector>
#include <string>
#include <functional>
#include "GmpiApiCommon.h"

// GOAL is that DAW-facing parameters are real-time safe and available via atomics that always updated immediatly, other parameters can be updated asyncronously since they are pretty much invisible to the
// DAW.
// update DAW facing atomics first
// save remainder for processor and trigger an interrupt for it to read it.
// processor should respond to interrupt by loading the preset, with the atomics taking precendence. (in case they were update by DAW calling setParameter in the mean time.


// TODO!!! Need to cope with ignore-program-change flag.
// need to update atomics immediatly in controller.
// need to handle setParameter from DAW.

struct paramInfo
{
	std::vector< std::string > defaultRaw;
	gmpi::PinDatatype dataType = gmpi::PinDatatype::Float32;
	double minimum = 0.0;
	double maximum = 10.0;
	std::string meta; // enum list/file ext
	bool private_ = false;
	bool ignoreProgramChange = false;

#ifdef _DEBUG
	std::string name;
#endif
};

struct paramValue
{
	std::vector< std::string > rawValues_; // rawValues_[voice] where voice is 0 - 127
	gmpi::PinDatatype dataType = gmpi::PinDatatype::Float32;
};

struct DawPreset
{
	DawPreset(const std::map<int32_t, paramInfo>& parameters, std::string presetString, int presetIdx = 0);
	DawPreset(const std::map<int32_t, paramInfo>& parameters, class TiXmlNode* presetXml, int presetIdx = 0);
	DawPreset(const DawPreset& other);
	DawPreset() {}

	void initFromXML(const std::map<int32_t, paramInfo>& parametersInfo, TiXmlNode* presetXml, int presetIdx);

	std::string toString(int32_t pluginId, std::string presetNameOverride = {}) const;

//	int idx = 0; // 'generation'
	std::string name;
	std::string category;
	std::map<int32_t, paramValue> params;
	std::size_t hash = 0;
	mutable bool ignoreProgramChangeActive = false;
};

// handles setting parameters and presets in a thread-safe manner in the presence of a hostile DAW.
// The idea is to ring-fence all the messyness of DAW parameter setting and preset loading into this class.
class DawStateManager
{
	struct nativeParameter
	{
		std::atomic<float> valueNormalized;
		int32_t index;

		nativeParameter(int32_t i) : index(i)
		{
		}
	};

	std::vector< std::unique_ptr<nativeParameter> > nativeParameters;

	std::map<int32_t, paramInfo> parameterInfos;

	std::vector< std::unique_ptr<const DawPreset> > presets;

	std::atomic<int> currentPresetIndex = -1;
	std::mutex presetMutex;
	bool ignoreProgramChange = false;

public:
	std::vector < std::function<void(DawPreset const*)>> callbacks;

	void init(class TiXmlElement* parameters_xml);

	void setNormalizedUnsafe(int paramIndex, float value)
	{
		nativeParameters[paramIndex]->valueNormalized.store(value, std::memory_order_relaxed);
	}

	float getNormalized(int paramIndex)
	{
		return nativeParameters[paramIndex]->valueNormalized.load(std::memory_order_relaxed);
	}

	DawPreset const* retainPreset(DawPreset const* preset);
	void setPreset(const std::string& presetString);
	void setPreset(DawPreset const* preset);
//	void setPresetFromFile(const char* filename, int presetIndex = 0);
	DawPreset const* fileToPreset(const char* filename, int presetIndex = 0);

	void enableIgnoreProgramChange()
	{
		ignoreProgramChange = true;
	}
};