#pragma once
#include <vector>
#include "dsp_msg_target.h"
#include "QueClient.h"
#include "se_types.h"
#include "HostControls.h"
#include "modules/shared/xplatform.h"
#include "modules/shared/RawView.h"

class my_input_stream;
class my_output_stream;
class UPlug;
class IDspPatchManager;

typedef std::vector<class PatchStorageBase*> patchMemory_t;

class dsp_patch_parameter_base :
	public dsp_msg_target, public QueClient
{
public:
	dsp_patch_parameter_base();
	virtual ~dsp_patch_parameter_base();

	bool SetValueRaw2(const void* data, int size, int patch, int voice);
	bool SetValueRaw3(RawView raw, int patch = 0, int voice = 0)
	{
		return SetValueRaw2(raw.data(), static_cast<int>(raw.size()), patch, voice);
	}
	int GetValueRaw(const void*& returnData, int patch = 0, int voice = 0);
	RawView GetValueRaw2(int patch = 0, int voice = 0);
	void UpdateUI(bool due_to_vst_automation_from_dsp_thread = false, int voice = 0);
	virtual void vst_automate(timestamp_t p_timestamp, int voiceId, float p_normalised_val, bool isMidiMappedAutomation) = 0;
	virtual bool typeIsVariableSize() = 0;
	virtual int typeSize() = 0;
	virtual int dataType() = 0;
	virtual void Initialize( class TiXmlElement* xml );
	virtual bool SetValueFromXml(const std::string& valueString, int voice, int patch) = 0;
	virtual std::string GetValueAsXml(int voice = 0 ) = 0;
#if defined( _DEBUG )
	virtual std::wstring GetValueString(int patch) = 0;
#endif
	virtual void SerialiseValue( my_output_stream& p_stream, int voice, int patch );
	virtual void SerialiseMetaData( my_input_stream& p_stream ) = 0;
	virtual void SerialiseValue( my_input_stream& p_stream, int voice, int patch );
	virtual void CopyPlugValue( int voice, UPlug* p_plug) = 0;
	const void* SerialiseForEvent(int voice, int& size);
	void OnUiMsg(int p_msg_id, my_input_stream& p_stream);
	void setPatchMgr(IDspPatchManager* p_patch_mgr)
	{
		m_patch_mgr=p_patch_mgr;
	}

	bool hasNormalized()
	{
		return !typeIsVariableSize(); // Exclude text and blobs.
	}
	virtual float GetValueNormalised( int /*voiceId*/ = 0 )
	{
		return 0.f;
	}
	virtual void setValueNormalised(float p_normalised) = 0;

	void setUpFromDsp( struct parameter_description* parameterDescription, class InterfaceObject* pinDescription );
	void UpdateOutputParameter(int voice, UPlug* p_plug);
	inline bool isPolyphonic()
	{
		return isPolyphonic_;
	}
	bool isTiming();
	// gate is special as gate-on must first allocate new voice.
	bool isPolyphonicGate()
	{
		return isPolyphonicGate_;
	}

	void OnValueChanged(int voiceId = 0, timestamp_t time = -1, bool p_due_to_program_change = false, bool initialUpdate = false);
	void OnValueChangedFromGUI(bool p_due_to_program_change, int voiceId, bool initialUpdate = false);
	void SendValue(timestamp_t timestamp, int voiceId);
	void SendValuePt2( timestamp_t unadjusted_timestamp, class Voice* voice = 0, bool isInitialUpdate = false);
	void SendValuePoly(timestamp_t unadjusted_timestamp, int physicalVoiceNumber, RawView value, bool isInitialUpdate = false);
	void SetGrabbed(bool pGrabbed);

	void RegisterHandle( int handle );

	// New way to connect to module.
	void ConnectPin2( UPlug* toPin );

	void setAutomation(int controller_id, bool notifyUI = false);
	int UnifiedControllerId()
	{
		return m_controller_id;
	}
	std::wstring getAutomationSysex()
	{
		return m_controller_sysex;
	}
	void setAutomationSysex(std::wstring sysex)
	{
		m_controller_sysex = sysex;
	}
	void setPolyphonic(bool isPolyphonic)
	{
		isPolyphonic_=isPolyphonic;
	}
	void setPolyphonicGate(bool isPolyphonicGate)
	{
		isPolyphonicGate_=isPolyphonicGate;
	}
	void setHostControlId(HostControls id)
	{
		hostControlId_ = id;
	}
	void setmoduleHandle(int id)
	{
		moduleHandle_ = id;
	}
	void setmoduleParameterId(int id)
	{
		moduleParameterId_ = id;
	}

	HostControls getHostControlId() const
	{
		return hostControlId_;
	}
	int getModuleHandle()
	{
		return moduleHandle_;
	}
	int getModuleParameterId()
	{
		return moduleParameterId_;
	}

	// Polyphonic host-controls associate with their container.
	inline class ug_container* getVoiceContainer()
	{
		return voiceContainer_;
	}
	inline void setVoiceContainer(ug_container* c)
	{
		voiceContainer_ = c;
	}
	inline int getVoiceContainerHandle()
	{
		return voiceContainerHandle_;
	}
	virtual void InitializePatchMemory( const wchar_t* defaultString ) = 0;
	void createPatchMemory( /*int patchCount,*/ int voiceCount );
	void setSdk2BackwardCompatibility(bool v)
	{
		Sdk2BackwardCompatibilityFlag_ = v;
	}
	void vst_automate2(timestamp_t timestamp, int voice, const void* data, int size, bool isMidiMappedAutomation);
int EffectivePatch() const
{
	return 0;
}

	// QueClient support.
	virtual int queryQueMessageLength( int availableBytes );
	virtual void getQueMessage( my_output_stream& outStream, int messageLength );

	virtual void setMetadataRangeMinimum( double rangeMinimum ) = 0;
	virtual void setMetadataRangeMaximum( double rangeMaximum ) = 0;
	virtual void setMetadataTextMetadata( std::wstring metadata ) = 0;

	void outputMidiAutomation(bool p_due_to_program_change, int voiceId);

#if defined( _DEBUG )
	std::string debugName;
#endif

	int WavesParameterIndex = -1; // plugins.

	bool requiresAsyncRestart() const
	{
		switch (hostControlId_)
		{
		case HC_POLYPHONY:
		case HC_POLYPHONY_VOICE_RESERVE:
		case HC_OVERSAMPLING_RATE:
		case HC_OVERSAMPLING_FILTER:
		case HC_PATCH_CABLES:
		case HC_SILENCE_OPTIMISATION:
			return true;
			break;

		default:
			break;
		}
		return false;
	}

	bool persistAcrossResets() const
	{
		switch (hostControlId_)
		{
		case HC_POLYPHONY:
		case HC_POLYPHONY_VOICE_RESERVE:
		case HC_OVERSAMPLING_RATE:
		case HC_OVERSAMPLING_FILTER:
		case HC_PATCH_CABLES:
		case HC_SILENCE_OPTIMISATION:
		case HC_VOICE_PITCH:
			return true;
			break;

		default:
			break;
		}
		return false;
	}

	bool isStateFull(bool saveRestartState) const
	{
		return stateful_ ||
			// input parameters that may not be stateful, but do affect the DSP (e.g. Text-Setting modules) and need to be restored when changing sample-rate etc.
			(saveRestartState && ((hostControlId_ == HC_NONE && !outputPins_.empty()) || persistAcrossResets()));
	}

	bool ignorePatchChange()
	{
		return ignorePc;
	}

protected:
	IDspPatchManager* m_patch_mgr;
	int m_controller_id;
	std::wstring m_controller_sysex;
	bool m_grabbed;
	bool isPolyphonic_;
	bool isPolyphonicGate_;
	HostControls hostControlId_;
	patchMemory_t patchMemory;
	// some parameter updates are from host (don't reflect them back)
	// others are from MIDI or user, send those to host.
	bool hostNeedsParameterUpdate;
	bool Sdk2BackwardCompatibilityFlag_;
public:
	std::vector<class UPlug*> outputPins_; // output pins on the patch-automator/s

protected:
	// Avoid sending floods of repeated MIDI msgs on tiny parameter changes
	std::vector<int> lastMidiValue_;
	bool isDspOnlyParameter_;
	int voiceContainerHandle_; // Polyphonic host-controls associate with their container.
	ug_container* voiceContainer_;

	int32_t moduleHandle_;
	int32_t moduleParameterId_;
	bool stateful_ = true;
	bool ignorePc = false;
};
