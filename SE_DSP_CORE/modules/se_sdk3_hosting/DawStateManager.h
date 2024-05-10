#pragma once

/*
#include "DawStateManager.h"
*/

#include "ProcessorStateManager.h"

// GOAL is that DAW-facing parameters are real-time safe and available via atomics that always updated immediatly, other parameters can be updated asyncronously since they are pretty much invisible to the
// DAW.
// update DAW facing atomics first
// save remainder for processor and trigger an interrupt for it to read it.
// processor should respond to interrupt by loading the preset, with the atomics taking precendence. (in case they were update by DAW calling setParameter in the mean time.


// TODO!!! Need to cope with ignore-program-change flag.
// need to update atomics immediatly in controller.
// need to handle setParameter from DAW.


// handles setting parameters and presets in a thread-safe manner in the presence of a hostile DAW.
// The idea is to ring-fence all the messiness of DAW parameter setting and preset loading into this class.
class DawStateManager : public ProcessorStateMgrVst3
{
	/*
	struct nativeParameter
	{
//	private:
//		int32_t index;
	public:
		std::atomic<float> valueNormalized;

		nativeParameter(int32_t) //: index(i)
		{
		}
	};

	std::vector< std::unique_ptr<nativeParameter> > nativeParameters;
	*/

//	std::atomic<int> currentPresetIndex = -1;

public:
//	std::vector < std::function<void(DawPreset const*)>> callbacks;

//	void init(class TiXmlElement* parameters_xml);

/*
	void setNormalizedUnsafe(int paramIndex, float value)
	{
		nativeParameters[paramIndex]->valueNormalized.store(value, std::memory_order_relaxed);
	}
	float getNormalized(int paramIndex)
	{
		return nativeParameters[paramIndex]->valueNormalized.load(std::memory_order_relaxed);
	}
*/
	
	void setPresetFromUnownedPtr(DawPreset const* preset);

//	DawPreset const* retainPreset(DawPreset const* preset);
//	void setPresetFromXml(const std::string& presetString);
//	void setPreset(DawPreset const* preset);
	DawPreset const* fileToPreset(const char* filename, int presetIndex = 0);
	DawPreset const* xmlToPreset(std::string xml, bool retain = true);
};