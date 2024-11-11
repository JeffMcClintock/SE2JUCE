#pragma once
#include "IDspPatchManager.h"

// Used by oversampling containers as bridge to real patchmanager in higher container.
class DspPatchManagerProxy :
	public IDspPatchManager
{
public:
	DspPatchManagerProxy(class ug_container* p_container, class ug_oversampler* oversampler);
	~DspPatchManagerProxy();

	void InitializeAllParameters() override;
	void OnMidi(VoiceControlState* voiceState, timestamp_t timestamp, const unsigned char* midiMessage, int size, bool fromMidiCv) override;
	float InitializeVoiceParameters(ug_container* voiceControlContainer, timestamp_t timestamp, Voice* voice /*int voiceId, bool hardReset*/, bool sendTrigger) override;// , bool patchManagerAllocatesVoices);
	void SendInitialUpdates() override;
	void OnUiMsg(int p_msg_id, my_input_stream& p_stream) override;
	void vst_Automation(ug_container* voiceControlContainer, timestamp_t p_clock, int p_controller_id, float p_normalised_value, bool sendToMidiCv = true, bool sendToNonMidiCv = true) override;
	void vst_Automation2(timestamp_t p_clock, int p_controller_id, const void* data, int size) override;
#if defined(SE_TARGET_PLUGIN)
	void setParameterNormalized( timestamp_t p_clock, int vstParameterIndex, float newValue, int32_t flags ) override; // VST3.
	void setParameterNormalizedDaw(timestamp_t p_clock, int32_t paramHandle, float newValue, int32_t flags) override;
#endif
	void setPreset(struct DawPreset const* preset) override;
	void getPresetState( std::string& chunk, bool saveRestartState) override;
    void setPresetState( const std::string& chunk, bool overrideIgnoreProgramChange = false) override;
	class dsp_patch_parameter_base* GetHostControl(int32_t hostControl, int32_t attachedToContainerHandle = -1) override;
	class dsp_patch_parameter_base* createPatchParameter( int typeIdentifier ) override; // from GUI.

	virtual	dsp_patch_parameter_base* ConnectHostControl(HostControls hostConnect, UPlug* plug) override;
	void ConnectHostControl2(HostControls hostConnect, UPlug * toPlug) override;
	dsp_patch_parameter_base* ConnectParameter(int parameterHandle, UPlug* plug) override;
	struct FeedbackTrace* InitSetDownstream(ug_container * voiceControlContainer) override;

	void setMidiChannel( int c ) override;
	int getMidiChannel( ) override;
	void setMidiCvVoiceControl() override;
	void vst_setAutomationId(dsp_patch_parameter_base* p_param, int p_controller_id) override;
	class ug_container* Container() override;
	void setupContainerHandles(ug_container* /*subContainer*/) override;
	dsp_patch_parameter_base* GetParameter(int moduleHandle, int paramIndex) override;

private:
	ug_oversampler* oversampler_;
};

