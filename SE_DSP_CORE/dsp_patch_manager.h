#pragma once
#include <map>
#include <vector>
#include <fstream>
#include "dsp_msg_target.h"
#include "IDspPatchManager.h"
#include "modules/se_sdk3/mp_midi.h"

// #define DEBUG_LOG_PM_TO_FILE 1

typedef std::vector<dsp_patch_parameter_base*> dsp_parameters_type;
typedef std::multimap<int,dsp_patch_parameter_base*> dsp_automation_map_type;

class DspPatchManager : public IDspPatchManager
{
public:
#ifdef DEBUG_LOG_PM_TO_FILE
	std::ofstream loggingFile;
	inline static std::ofstream* currentLoggingFile = {};
#endif

	DspPatchManager( ug_container* p_container );
	~DspPatchManager();

	void setupContainerHandles(ug_container* subContainer) override;
	//void setProgramDspThread( int program ) override;
	void vst_Automation(ug_container* voiceControlContainer, timestamp_t p_clock, int p_controller_id, float p_normalised_value, bool sendToMidiCv = true, bool sendToNonMidiCv = true) override;
	void SendInitialUpdates() override;
	void setMidiChannel( int c ) override;
	int getMidiChannel( ) override
	{
		return midiChannel_;
	}
	void OnUiMsg(int p_msg_id, my_input_stream& p_stream) override;
	void vst_setAutomationId(dsp_patch_parameter_base* p_param, int p_controller_id) override;

	virtual class dsp_patch_parameter_base* GetParameter( int handle );
	virtual dsp_patch_parameter_base* GetParameter( int moduleHandle, int paramIndex ) override;
	virtual dsp_patch_parameter_base* createPatchParameter( int typeIdentifier ) override; // from GUI.

	// new.
	virtual	dsp_patch_parameter_base* ConnectHostControl(HostControls hostConnect, UPlug* plug) override;
	void ConnectHostControl2(HostControls hostConnect, UPlug * toPlug) override;
	struct FeedbackTrace* InitSetDownstream(ug_container * voiceControlContainer) override;
	virtual	dsp_patch_parameter_base* ConnectParameter(int parameterHandle, UPlug* plug) override;

	ug_container* Container() override
	{
		return m_container;
	}
	virtual void setMidiCvVoiceControl() override
	{
		MidiCvControlsVoices_ = true;
	}
	virtual void OnMidi(VoiceControlState* voiceState, timestamp_t timestamp, const unsigned char* midiMessage, int size, bool fromMidiCv) override;
	void MidiSetMonoMode(timestamp_t timestamp, bool newMonoMode);
	float InitializeVoiceParameters(ug_container* voiceControlContainer, timestamp_t timestamp, Voice* voice, /*int voiceId, bool hardReset,*/ bool sendTrigger) override;
	void vst_Automation2(timestamp_t p_clock, int p_controller_id, const void* data, int size ) override;

	class dsp_patch_parameter_base* GetHostControl(int32_t hostControl, int32_t attachedToContainerHandle = -1) override;
#if defined(SE_TARGET_PLUGIN)
	void setParameterNormalized( timestamp_t p_clock, int vstParameterIndex, float newValue, int32_t flags ) override; // VST3.
	void setParameterNormalizedDaw(timestamp_t p_clock, int32_t paramHandle, float newValue, int32_t flags) override;  // JUCE
#endif

	void setPreset(struct DawPreset const* preset) override;
	void setPresetState( const std::string& chunk, bool overrideIgnoreProgramChange = false) override;
	void getPresetState(std::string& chunk, bool saveRestartState)  override;

	void InitializeAllParameters() override;

protected:
	void AddParam( dsp_patch_parameter_base* p_param );
//	void UpdateProgram( int program );
	void DoNoteOn(timestamp_t timestamp, class ug_container* voiceControlContainer, int voiceId, float velocity);
	void DoNoteOff(timestamp_t timestamp, class ug_container* voiceControlContainer, int voiceId, float velocity);

	dsp_patch_parameter_base* GetParameter(ug_container* voiceControlContainer, HostControls hostConnect);

private:
	dsp_parameters_type m_parameters;
	dsp_parameters_type m_poly_parameters_cache; // sub-set for faster iterating of poly params.
	dsp_patch_parameter_base* vst_learn_parameter; // midi_learn.
	dsp_automation_map_type vst_automation_map;
	ug_container* m_container;
	int midiChannel_;
	// this just increments each time a voice is reset.
	int nextVoiceReset_ = 1;

	// used only in plugins, but harmless in editor.
	std::vector<dsp_patch_parameter_base*> parameterIndexes_; // VST-Host's parameter ID's.

	short two_byte_controler_value[32]; // store current LSB/MSB info from these controllers
	unsigned short incoming_rpn;
	unsigned short incoming_nrpn;
	int highResolutionVelocityPrefix;

	std::map<int, int> m_rpn_memory;
	bool MidiCvControlsVoices_;
	bool mEmulateIgnoreProgramChange = false;
};
