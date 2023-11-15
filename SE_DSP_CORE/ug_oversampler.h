#pragma once
#include <vector>
#include <list>
#include "ug_base.h"
#include "SeAudioMaster.h"

class ug_oversampler :
	public ug_base, public AudioMasterBase
{
public:
	static const int CONTAINER_FIXED_PLUG_COUNT = 3;

	ug_oversampler();
	DECLARE_UG_BUILD_FUNC(ug_oversampler);
	virtual void Resume() override;
	virtual void Wake() override;
	ug_base* Clone( CUGLookupList& UGLookupList ) override;
	void Setup1(int factor, int poles, bool copyStandardPlugs = false );
	void Setup1b(int factor, int poles, bool copyStandardPlugs = false );
	void Setup2(bool xmlMethod);
	void CreatePatchManagerProxy();
	virtual int Open() override;
	virtual int Close() override;
	virtual void DoProcess( int buffer_offset, int sampleframes ) override;
	void HandleEvent(SynthEditEvent* e) override;
	void SendPinsInitialStatus();
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

	void RelayOutGoingEvent( UPlug* outerPlug, SynthEditEvent* e );
	void RelayOutGoingState( timestamp_t timestamp, UPlug* outerPlug, state_type new_state );
	UPlug* CreateProxyPlug( UPlug* destination );

	// Hosting overrides.
	int BlockSize() override;
	timestamp_t NextGlobalStartClock() override;
	ISeShellDsp* getShell() override;
	void InsertBlockDev(EventProcessor* p_dev) override;
	void SuspendModule(ug_base* p_module) override;
	void RegisterDspMsgHandle(dsp_msg_target* p_object, int p_handle) override;
	void UnRegisterDspMsgHandle(int p_handle) override;
	void AssignTemporaryHandle(dsp_msg_target* p_object) override;
	dsp_msg_target* HandleToObject(int p_handle) override;
	int getBlockPosition() override;
	timestamp_t BlockStartClock() override;
	int latencyCompensationMax() override;
	bool SilenceOptimisation() override
	{
		return AudioMaster()->SilenceOptimisation();
	}
	timestamp_t CalculateOversampledTimestamp( ug_container* top_container, timestamp_t timestamp ) override;
	float SampleRate() override;
	timestamp_t SampleClock() override;
	void Wake(EventProcessor* ug) override;
	void ugCreateSharedLookup(const std::wstring& id, LookupTable** lookup_ptr, int sample_rate, size_t p_size, bool integer_table, bool create=true, SharedLookupScope scope = SLS_ONE_MODULE ) override;
	void ServiceGuiQue() override;
	void RegisterPatchAutomator( class ug_patch_automator* client ) override;

#if defined( SE_FIXED_POOL_MEMORY_ALLOCATION )
	struct SynthEditEvent* AllocateMessage(timestamp_t p_timeStamp = 0, int32_t p_eventType = 0, int32_t p_parm1 = 0, int32_t p_parm2 = 0, int32_t p_parm3 = 0, int32_t p_parm4 = 0, char* extra = 0)
	{
		return AudioMaster()->AllocateMessage(p_timeStamp , p_eventType , p_parm1 , p_parm2 , p_parm3 , p_parm4, extra );
	}
	void DeAllocateMessage(SynthEditEvent* e)
	{
		AudioMaster()->DeAllocateMessage(e);
	}
	char* AllocateExtraData(uint32_t lSize)
	{
		return AudioMaster()->AllocateExtraData(lSize);
	}
	void DeAllocateExtraData(char* pExtraData)
	{
		AudioMaster()->DeAllocateExtraData(pExtraData);
	}
#endif

	// Oddball.
	int CallVstHost(int opcode, int index, int value, void* ptr, float opt) override;

	// ISeAudioMaster support.
	void onFadeOutComplete() override;
	void SetPPVoiceNumber(int n) override;

	void OnCpuMeasure() override;

	int filterSetting()
	{
		return filterType_;
	}

	inline timestamp_t upsampleTimestamp(timestamp_t timeStamp)
	{
		return (timeStamp - clientClockOffset_) / oversampleFactor_;
	}

	int calcDelayCompensation() override;
	void IterateContainersDepthFirst(std::function<void(ug_container*)>& f) override;
	void SetModuleLatency(int32_t handle, int32_t latency) override;
	int32_t RegisterIoModule(class ISpecialIoModule*) override { return 1; } // can't oversample an IO module

	UPlug* routePatchCableOut(UPlug* plug);
	void ReRoutePlugs() override;

private:

	int oversampleFactor_;
	int filterType_;

#if defined( _DEBUG )
	// old way. kept in debug for verification new way works.
	timestamp_t clientClockAtMyBlockStart;
#endif
	timestamp_t clientSampleClock;
	timestamp_t clientBlockStartClock;
	timestamp_t clientNextGlobalStartClock_;
	timestamp_t moduleInsertClock_;

	timestamp_t clientClockOffset_;

public: // for diagnostic cancelation test.
	class ug_oversampler_out* oversampler_out;
	class ug_oversampler_in* oversampler_in;

private:
	// maps container plug index to oversampler I/O module plugs.
	std::vector<UPlug*> InsidePlugs;
	class DspPatchManagerProxy* dspPatchManager;
	int bufferPositionOversampled;
};

