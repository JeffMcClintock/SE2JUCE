#include "pch.h"
#include "assert.h"
#include "DspPatchManagerProxy.h"
#include "ug_container.h"
#include "ug_oversampler.h"
#include "./ug_patch_param_watcher.h"
#include "./dsp_patch_parameter_base.h"

DspPatchManagerProxy::DspPatchManagerProxy(ug_container* /*patch_control_container*/, ug_oversampler* oversampler) :
	oversampler_(oversampler)
{
}

DspPatchManagerProxy::~DspPatchManagerProxy()
{
}

void DspPatchManagerProxy::InitializeAllParameters()
{
	// ok, no params of my own.	assert(false);
}

void DspPatchManagerProxy::setMidiCvVoiceControl()
{
	// WE are in an oversampled container with no Patch-Automator. Nothing to do.
}

void DspPatchManagerProxy::OnMidi(VoiceControlState* voiceState, timestamp_t timestamp, const unsigned char* midiMessage, int size, bool fromMidiCv)
{
#ifdef _DEBUG
	const auto oversampleRate = (int)(voiceState->voiceControlContainer_->getSampleRate() / oversampler_->ParentContainer()->getSampleRate());
	assert(timestamp == (timestamp / oversampleRate) * oversampleRate);
#endif

	oversampler_->get_patch_manager()->OnMidi(voiceState, oversampler_->upsampleTimestamp(timestamp), midiMessage, size, fromMidiCv);
}

float DspPatchManagerProxy::InitializeVoiceParameters(ug_container* voiceControlContainer, timestamp_t timestamp, Voice* voice /*int voiceId, bool hardReset*/, bool sendTrigger)//, bool patchManagerAllocatesVoices)
{
	// BUG: if timestamp is not a multiple of oversample rate, it will get rounded down by the time it's re-converted.
	// e.g.upsampleTimestamp(100 at 8x) -> 12 --downsampled--> 96 !!! = assertion (past timestamp)
	/*
	ug_patch_param_setter::HandleEvent (UET_GENERIC1)
	   parent_container->PlayWaitingNotes( e->timeStamp );
	      VoiceList::DoNoteOn()
		     DspPatchManagerProxy::InitializeVoiceParameters ..
			    dsp_patch_parameter_base::SendValuePoly with downsampled timestamp
	*/
#ifdef _DEBUG
	const auto oversampleRate = (int) (voiceControlContainer->getSampleRate() / oversampler_->ParentContainer()->getSampleRate() );
	assert(timestamp == (timestamp / oversampleRate) * oversampleRate);
#endif

	return oversampler_->get_patch_manager()->InitializeVoiceParameters(voiceControlContainer, oversampler_->upsampleTimestamp(timestamp), voice /*int voiceId, bool hardReset*/, sendTrigger);
}

void DspPatchManagerProxy::vst_Automation(ug_container* voiceControlContainer, timestamp_t timestamp, int p_controller_id, float p_normalised_value, bool sendToMidiCv, bool sendToNonMidiCv)
{
#ifdef _DEBUG
	const auto oversampleRate = (int)(voiceControlContainer->getSampleRate() / oversampler_->ParentContainer()->getSampleRate());
	assert(timestamp == (timestamp / oversampleRate) * oversampleRate);
#endif

	return oversampler_->get_patch_manager()->vst_Automation(voiceControlContainer, oversampler_->upsampleTimestamp(timestamp), p_controller_id, p_normalised_value, sendToMidiCv, sendToNonMidiCv);
}

void DspPatchManagerProxy::vst_Automation2(timestamp_t /*p_clock*/, int /*p_controller_id*/, const void* /*data*/, int /*size*/)
{
	assert(false);
}

void DspPatchManagerProxy::SendInitialUpdates()
{
	assert(false);
}
void DspPatchManagerProxy::OnUiMsg(int /*p_msg_id*/, my_input_stream& /*p_stream*/)
{
	assert(false);
}

#if defined(SE_TARGET_PLUGIN)
void DspPatchManagerProxy::setParameterNormalized( timestamp_t p_clock, int vstParameterIndex, float newValue ) // VST3.
{
	assert(false);
}
#endif

void DspPatchManagerProxy::setPresetState( const std::string& /*chunk*/)
{
	assert(false);
}

void DspPatchManagerProxy::getPresetState( std::string& /*chunk*/, bool /*saveRestartState*/)
{
	assert(false);
}
#if defined(SE_TARGET_PLUGIN)
#endif

dsp_patch_parameter_base* DspPatchManagerProxy::ConnectHostControl(HostControls hostConnect, UPlug* plug)
{
	return oversampler_->get_patch_manager()->ConnectHostControl(hostConnect, plug);
}

void DspPatchManagerProxy::ConnectHostControl2(HostControls hostConnect, UPlug * toPlug)
{
	oversampler_->get_patch_manager()->ConnectHostControl2(hostConnect, toPlug);
}

dsp_patch_parameter_base* DspPatchManagerProxy::ConnectParameter(int parameterHandle, UPlug* plug)
{
	return oversampler_->get_patch_manager()->ConnectParameter(parameterHandle, plug);
}

struct FeedbackTrace* DspPatchManagerProxy::InitSetDownstream(ug_container * voiceControlContainer)
{
	return oversampler_->get_patch_manager()->InitSetDownstream(voiceControlContainer);
}

class dsp_patch_parameter_base* DspPatchManagerProxy::createPatchParameter( int /*typeIdentifier*/ )
{
	assert(false);    // from GUI.
	return 0;
}

//void DspPatchManagerProxy::setProgram( int /*program*/ )
//{
//	assert(false);
//}
//int DspPatchManagerProxy::getProgram()
//{
//	assert(false);
//	return 0;
//}
//void DspPatchManagerProxy::setProgramDspThread( int /*program*/ )
//{
//	assert(false);
//}
void DspPatchManagerProxy::setMidiChannel( int )
{
	assert(false);
}
int DspPatchManagerProxy::getMidiChannel( )
{
	assert(false);
	return 0;
}

void DspPatchManagerProxy::vst_setAutomationId(dsp_patch_parameter_base* /*p_param*/, int /*p_controller_id*/)
{
	assert(false);
}
class ug_container* DspPatchManagerProxy::Container()
{
	assert(false);
	return 0;
}

dsp_patch_parameter_base* DspPatchManagerProxy::GetParameter(int moduleHandle, int paramIndex)
{
	return oversampler_->get_patch_manager()->GetParameter(moduleHandle, paramIndex);
}

dsp_patch_parameter_base* DspPatchManagerProxy::GetHostControl(int hostControl)
{
	return oversampler_->get_patch_manager()->GetHostControl(hostControl);
}

#if 0
// Good idea - rerouting parameters via intervening oversamplers. but didn't account for polyphinic parameters.
void DspPatchManagerProxy::RegisterParameterDestination(int32_t parameterHandle, UPlug* destinationPin)
{
	oversampler_->routeParameterOut(parameterHandle, destinationPin);
}
#endif