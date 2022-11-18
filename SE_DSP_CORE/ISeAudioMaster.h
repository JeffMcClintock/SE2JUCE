/*
#include "SeAudioMaster.h"
*/
#if !defined(ISEAUDIOMASTER_H_INCLUDED_)
#define ISEAUDIOMASTER_H_INCLUDED_

#pragma once
#include "se_types.h"

enum SharedLookupScope { SLS_ONE_MODULE, SLS_ALL_MODULES };

// generic host interface for modules.
class ISeAudioMaster
{
public:
	virtual float SampleRate() = 0;
	virtual int BlockSize() = 0;
	virtual timestamp_t SampleClock() = 0;
	virtual int getBlockPosition() = 0;
	virtual timestamp_t NextGlobalStartClock() = 0;
	virtual timestamp_t BlockStartClock() = 0;
	virtual int latencyCompensationMax() = 0;
	virtual bool SilenceOptimisation() = 0;
	virtual void InsertBlockDev(class EventProcessor* p_dev) = 0;
	virtual void SuspendModule(class ug_base* p_module) = 0;
	virtual void Wake(EventProcessor* ug) = 0;

	virtual void RegisterDspMsgHandle(class dsp_msg_target* p_object, int p_handle) = 0;
	virtual void UnRegisterDspMsgHandle(int p_handle) = 0;
	virtual void AssignTemporaryHandle(dsp_msg_target* p_object) = 0;
	virtual dsp_msg_target* HandleToObject(int p_handle) = 0;

	virtual class ISeShellDsp* getShell() = 0;

	virtual void ugCreateSharedLookup(const std::wstring& id, class LookupTable** lookup_ptr, int sample_rate, size_t p_size, bool integer_table, bool create=true, SharedLookupScope scope = SLS_ONE_MODULE  ) = 0;
	virtual void ServiceGuiQue() = 0;

	// Oddball.
	virtual void RegisterPatchAutomator( class ug_patch_automator* client ) = 0;
	virtual timestamp_t CalculateOversampledTimestamp( class ug_container* top_container, timestamp_t timestamp ) = 0;
	virtual int CallVstHost(int opcode, int index = 0, int value = 0, void* ptr = 0, float opt = 0) = 0;
	virtual void onFadeOutComplete() = 0;

#if defined( SE_FIXED_POOL_MEMORY_ALLOCATION )
	// Waves fixed pool memory allocation.
	virtual struct SynthEditEvent* AllocateMessage(timestamp_t p_timeStamp = 0, int32_t p_eventType = 0, int32_t p_parm1 = 0, int32_t p_parm2 = 0, int32_t p_parm3 = 0, int32_t p_parm4 = 0, char* extra = 0) = 0;
	virtual void DeAllocateMessage(SynthEditEvent* message) = 0;
	virtual char* AllocateExtraData(uint32_t lSize) = 0;
	virtual void DeAllocateExtraData(char* pExtraData) = 0;
#else
	inline SynthEditEvent* AllocateMessage(timestamp_t p_timeStamp = 0, int32_t p_eventType = 0, int32_t p_parm1 = 0, int32_t p_parm2 = 0, int32_t p_parm3 = 0, int32_t p_parm4 = 0, char* extra = 0)
	{
		return new SynthEditEvent(p_timeStamp, p_eventType, p_parm1, p_parm2, p_parm3, p_parm4, extra);
	}
	inline static char* AllocateExtraData(uint32_t lSize)
	{
		return new char[lSize];
	}
#endif // SE_FIXED_POOL_MEMORY_ALLOCATION

	virtual void RegisterModuleDebugger(class UgDebugInfo* module) = 0;
	virtual void AddCpuParent(ug_base* ug) = 0;
	virtual void SetModuleLatency(int32_t handle, int32_t latency) = 0;
	virtual int32_t RegisterIoModule(class ISpecialIoModule* m) = 0;
};

#endif // !defined(ISEAUDIOMASTER_H_INCLUDED_)
