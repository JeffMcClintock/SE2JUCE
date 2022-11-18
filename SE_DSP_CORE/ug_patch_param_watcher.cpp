#include "pch.h"
#include <vector>
#include "ug_patch_param_watcher.h"

#include "./dsp_patch_parameter_base.h"
#include "./ug_container.h"
#include "./midi_defs.h"
#include "module_register.h"
#include "ug_midi_automator.h"
#include "./IDspPatchManager.h"

SE_DECLARE_INIT_STATIC_FILE(ug_patch_param_watcher)

#define FIRST_PARAM_PLUG_IDX 3
#define PN_VOICE_ID 2


using namespace std;

// Fill an array of InterfaceObjects with plugs and parameters
void ug_patch_param_watcher::ListInterface2(InterfaceObjectArray& PList)
{
	// used to keep this module downstream of MIDI automator (no MIDI data ever sent)
	LIST_VAR3N( L"MIDI In",  DR_IN, DT_MIDI2 , L"0", L"", IO_DISABLE_IF_POS, L"");
	LIST_VAR3N(L"MIDI Out", DR_OUT, DT_MIDI2 , L"", L"", IO_DISABLE_IF_POS, L"");
	// provide VoiceId, attached to outgoing poly automation.
	LIST_VAR3( L"Voice/VirtualVoiceId", voiceId_, DR_IN, DT_INT , L"", L"", IO_HOST_CONTROL|IO_PAR_POLYPHONIC, L"");
}

namespace
{
REGISTER_MODULE_1( L"SE Patch Parameter Watcher", IDS_MN_PP_WATCHER,IDS_MG_DEBUG,ug_patch_param_watcher ,CF_STRUCTURE_VIEW,L"");
}

ug_patch_param_watcher::ug_patch_param_watcher() :
	voiceId_(0)
{
	/*
	Can't suspend param watcher because it needs to monitor both mono and poly parameters. At least can't suspend voice 0 or 1 (main one).
	mayby need a mono one on voice zero, and poly ones on other voices. !!!
	*/
	SetFlag(UGF_NEVER_SUSPEND /*|UGF_PARAMETER_WATCHER*/);
	SET_CUR_FUNC( &ug_base::process_sleep );
}

void ug_patch_param_watcher::CreatePlug(int32_t parameterHandle, UPlug* plug)
{
	UPlug* new_plug = new UPlug(this, DR_IN, plug->DataType);

	// this plug added after SetupDynamicPlugs(), so need to manually create buffers.
	new_plug->CreateBuffer();
	// ensure polyphonic output params handled correctly.
	new_plug->SetFlag(PF_PP_ACTIVE);
	AddPlug(new_plug);
	connect(plug, new_plug);
	assert(GetPlug(1) == GetPlug(L"MIDI Out"));

	// Ensure automation_output_device (if any) always downstream, so it can send MIDI on time.
	UPlug* midiPlug = GetPlug(1);
	if (!midiPlug->InUse()) // only need one connection.
	{
		// Polyphonic Oversampler with no Patch-Automator has no automation_output_device.
		ug_base* ao = ParentContainer()->patch_control_container->automation_output_device;

		if (ao)
		{
			// this->patch_control_container not initilased because not built normal way.
			assert(patch_control_container && patch_control_container->automation_output_device);
			connect(midiPlug, ao->GetPlug(L"Hidden Input"));
		}
	}

	dsp_patch_parameter_base* patch_param = 0;
	if ((plug->flags & PF_PATCH_STORE) != 0)
	{
		patch_param = parent_container->get_patch_manager()->ConnectParameter(parameterHandle, new_plug);
	}
	else
	{
		patch_param = parent_container->get_patch_manager()->ConnectHostControl((HostControls)parameterHandle, new_plug);
	}
	assert(patch_param);
	patchParams.push_back(patch_param);
}

void ug_patch_param_watcher::CreateParameterPlug(int32_t moduleHandle, int moduleParameterId, UPlug* plug)
{
	UPlug* new_plug = new UPlug(this, DR_IN, plug->DataType);

	// this plug added after SetupDynamicPlugs(), so need to manually create buffers.
	new_plug->CreateBuffer();
	// ensure polyphonic output params handled correctly.
	new_plug->SetFlag(PF_PP_ACTIVE);
	AddPlug(new_plug);
	connect(plug, new_plug);
	assert(GetPlug(1) == GetPlug(L"MIDI Out"));

	// Ensure automation_output_device (if any) always downstream, so it can send MIDI on time.
	UPlug* midiPlug = GetPlug(1);
	if (!midiPlug->InUse()) // only need one connection.
	{
		// Polyphonic Oversampler with no Patch-Automator has no automation_output_device.
		ug_base* ao = ParentContainer()->patch_control_container->automation_output_device;

		if (ao)
		{
			// this->patch_control_container not initilased because not built normal way.
			assert(patch_control_container && patch_control_container->automation_output_device);
			connect(midiPlug, ao->GetPlug(L"Hidden Input"));
		}
	}

	auto parameter = parent_container->get_patch_manager()->GetParameter(moduleHandle, moduleParameterId);
	//	dsp_patch_parameter_base* patch_param = 0;
	assert((plug->flags & PF_PATCH_STORE) != 0);
	{
//		patch_param = parent_container->get_patch_manager()->ConnectParameter(parameterHandle, new_plug);
		parameter->ConnectPin2(new_plug);
	}

	assert(parameter);
	patchParams.push_back(parameter);
}

void ug_patch_param_watcher::SetupDynamicPlugs()
{
	// need to call CreateBuffer on all datatypes, not just samples.
	SetupDynamicPlugs2();
}

void ug_patch_param_watcher::onSetPin(timestamp_t /*p_clock*/, UPlug* p_to_plug, state_type /*p_state*/)
{
	/*
	Problem: With polyphonic signals there's no priority system. Parameter value will reflect most recent update from any voice (could jump arround). Ideally the most-recent-note would override others.
	Would probly require a Host-Control - MonoNoteId. To reflect current 'last' note following current note-priority rules.
	*/
	if( p_to_plug->getPlugIndex() == PN_VOICE_ID )
	{
		// This voice allocated to a new note-number, bring patch-mem up-to-speed for polyphonic signals.
		for( size_t i = FIRST_PARAM_PLUG_IDX ; i < plugs.size() ; ++i )
		{
			if( (plugs[i]->flags & PF_PPW_POLYPHONIC) != 0 )
			{
				size_t paramNumber = i - FIRST_PARAM_PLUG_IDX;
				patchParams[ paramNumber ]->UpdateOutputParameter( voiceId_, plugs[i] );
			}
		}
	}

	assert( voiceId_ >=0 && voiceId_ < 128 );
	int paramNumber = p_to_plug->getPlugIndex() - FIRST_PARAM_PLUG_IDX;

	if( paramNumber >= 0 ) // First 3 plugs are not parameters.
	{
		int effectiveVoice = voiceId_;

		if( (p_to_plug->flags & PF_PPW_POLYPHONIC) == 0 )
			effectiveVoice = 0;

		assert(patchParams[paramNumber]);
		patchParams[ paramNumber ]->UpdateOutputParameter( effectiveVoice, p_to_plug );
	}
}

ug_base* ug_patch_param_watcher::Clone( CUGLookupList& UGLookupList )
{
	auto clone = (ug_patch_param_watcher*) ug_base::Clone( UGLookupList );

	// Copy list of parameters.
	clone->patchParams = patchParams;

	return clone;
}

void ug_patch_param_watcher::RemoveDuplicateConnections()
{
	// determin which params are monophonic. They need to have voice forced to zero.
	for (auto& pin : plugs)
	{
		if (!pin->connections.empty() && pin->connections.front()->UG->GetPolyphonic())
		{
			pin->SetFlag(PF_PPW_POLYPHONIC);
		}
	}

	if (m_clone_of == this)
		return;

	// Remove monophonic connections to cloned param watchers, since primary parameter watcher has them handled already.
	// Prevents enormous signal duplication in big synths. esp FlowMotion.
	auto pinIterator = plugs.begin() + FIRST_PARAM_PLUG_IDX;
	auto paramIterator = patchParams.begin();

	int pinIndex = FIRST_PARAM_PLUG_IDX;
	while (pinIterator != plugs.end())
	{
		auto pin = *pinIterator;

		if ((pin->flags & PF_PPW_POLYPHONIC) != 0)
		{
			pin->setIndex(pinIndex++);
			++pinIterator;
			++paramIterator;
		}
		else
		{
			pin->DeleteConnectors();
			pinIterator = plugs.erase(pinIterator);
			delete pin;
			paramIterator = patchParams.erase(paramIterator);
		}
	}
}