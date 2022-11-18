#pragma once
#include "IDspPatchManager.h"

// Used by oversampling containers as bridge to real patchmanager in higher container.
class DspPatchManagerProxy :
	public IDspPatchManager
{
public:
	DspPatchManagerProxy(class ug_container* p_container, class ug_oversampler* oversampler);
	~DspPatchManagerProxy();

	virtual void InitializeAllParameters() override;
	virtual void OnMidi(VoiceControlState* voiceState, timestamp_t timestamp, const unsigned char* midiMessage, int size, bool fromMidiCv) override;
	virtual float InitializeVoiceParameters(ug_container* voiceControlContainer, timestamp_t timestamp, Voice* voice /*int voiceId, bool hardReset*/, bool sendTrigger) override;// , bool patchManagerAllocatesVoices);
	virtual void SendInitialUpdates() override;
	virtual void OnUiMsg(int p_msg_id, my_input_stream& p_stream) override;
	void vst_Automation(ug_container* voiceControlContainer, timestamp_t p_clock, int p_controller_id, float p_normalised_value, bool sendToMidiCv = true, bool sendToNonMidiCv = true) override;
	virtual void vst_Automation2(timestamp_t p_clock, int p_controller_id, const void* data, int size) override;
#if defined(SE_TARGET_PLUGIN)
	virtual	void setParameterNormalized( timestamp_t p_clock, int vstParameterIndex, float newValue ) override; // VST3.
    virtual void setPresetState( const std::string& chunk, bool saveRestartState) override;
#endif

#if defined(SE_TARGET_PLUGIN)
	virtual void getPresetState( std::string& chunk, bool saveRestartState) override;
#endif
	virtual	class dsp_patch_parameter_base* GetHostControl(int hostControl) override;
	virtual class dsp_patch_parameter_base* createPatchParameter( int typeIdentifier ) override; // from GUI.

	virtual	dsp_patch_parameter_base* ConnectHostControl(HostControls hostConnect, UPlug* plug) override;
	void ConnectHostControl2(HostControls hostConnect, UPlug * toPlug) override;
	virtual	dsp_patch_parameter_base* ConnectParameter(int parameterHandle, UPlug* plug) override;
	struct FeedbackTrace* InitSetDownstream(ug_container * voiceControlContainer) override;

	virtual void setProgram( int program ) override;
	virtual int getProgram() override;
	virtual void setProgramDspThread( int program ) override;
	virtual void setMidiChannel( int c ) override;
	virtual int getMidiChannel( ) override;
	virtual void setMidiCvVoiceControl() override;
	virtual void vst_setAutomationId(dsp_patch_parameter_base* p_param, int p_controller_id) override;
	virtual class ug_container* Container() override;
	virtual void setupContainerHandles(ug_container* /*subContainer*/) override{}
	virtual dsp_patch_parameter_base* GetParameter(int moduleHandle, int paramIndex) override;

private:
	ug_oversampler* oversampler_;
};

