#include "pch.h"
#include "ug_patch_param_setter.h"
#include "module_register.h"

#include "ug_event.h"
#include "ug_container.h"
#include "IDspPatchManager.h"
#include "dsp_patch_parameter_base.h"

SE_DECLARE_INIT_STATIC_FILE(ug_patch_param_setter)

#if defined( _DEBUG )
#include "SeAudioMaster.h"
#endif

using namespace std;
using namespace gmpi;

// Fill an array of InterfaceObjects with plugs and parameters
void ug_patch_param_setter::ListInterface2(InterfaceObjectArray& PList)
{
	// used to keep this module downstream of MIDI automator (no MIDI data ever sent)
	LIST_VAR3N( L"MIDI In",  DR_IN, DT_MIDI2 , L"0", L"", 0, L"");
	LIST_VAR3N(L"MIDI Out", DR_OUT, DT_MIDI2 , L"", L"", 0, L"");
}

namespace
{
	REGISTER_MODULE_1( L"SE Patch Parameter Setter", IDS_MN_PP_SETTER,IDS_MG_DEBUG,ug_patch_param_setter ,CF_STRUCTURE_VIEW,L"");
}

ug_patch_param_setter::ug_patch_param_setter()
{
	// drum synths get voice numbers propagated back upstream, cause engine to
	// suspend controls along with voice. However that will cause crash when vst host trys to automate suspended control
	// better solution would be to keep all controls in voice zero!!
	SetFlag(/*UGF_NEVER_SUSPEND|*/UGF_PARAMETER_SETTER);
}

int ug_patch_param_setter::Open()
{
	SET_CUR_FUNC( &ug_base::process_sleep );
	auto r = ug_base::Open();

	RUN_AT(SampleClock(), &ug_patch_param_setter::SendPendingOutputChanges);

	if (ParentContainer()->isContainerPolyphonic())
	{
		VoiceRefreshPeriod_ = AudioMaster()->BlockSize() * 4;
		EventProcessor::AddEvent(new_SynthEditEvent(SampleClock() + VoiceRefreshPeriod_ * 2, UET_VOICE_REFRESH, 0, 0, 0, 0));
		SET_CUR_FUNC(&ug_base::process_nothing); // lighter than process_sleep
	}

	return r;
}

void ug_patch_param_setter::setPolyPin( timestamp_t clock, int plugNumber, int physicalVoiceNumber, int datatype, int32_t data_size, void* data )
{
	UPlug* plug = GetPlug( plugNumber );

	// All voices are connected to this one pin, need to send value only to correct voice.

	for( vector<UPlug*>::iterator it = plug->connections.begin(); it != plug->connections.end(); ++it )
	{
		UPlug* to_plug = *it;
		ug_base* ug = to_plug->UG;

		if(	ug->pp_voice_num == physicalVoiceNumber )
		{
			// Shouldn't really set voice-active on non-polyphonic modules in Voice 0.
			// That pin should remain at default value.
			if( ug->GetPolyphonic() )
			{
				ug->SetPinValue( clock, to_plug->getPlugIndex(), datatype, data, data_size );
			}
		}
	}
}

void ug_patch_param_setter::HandleEvent(SynthEditEvent* e)
{
	switch( e->eventType )
	{
	case UET_VOICE_REFRESH:
		{
			ParentContainer()->DoVoiceRefresh();
			EventProcessor::AddEvent(new_SynthEditEvent(SampleClock() + VoiceRefreshPeriod_, UET_VOICE_REFRESH, 0, 0, 0, 0));
		}
		break;

	case UET_PLAY_WAITING_NOTES:
		{
#if 1
			// get pointer from two 32-bit values.
			union notNice
			{
				ug_container* c;
				int32_t raw[2];
			} pointerToContainer;

			pointerToContainer.raw[0] = e->parm1;
			pointerToContainer.raw[1] = e->parm2;
			pointerToContainer.c->PlayWaitingNotes(e->timeStamp);
#else
			parent_container->PlayWaitingNotes( e->timeStamp );
#endif
		}
		break;

	default:
		ug_base::HandleEvent( e );
	};
}

void ug_patch_param_setter::SetupDynamicPlugs()
{
	// need to call CreateBuffer on all datatypes, not just samples.
	SetupDynamicPlugs2();
}

void ug_patch_param_setter::ConnectVoiceHostControl(ug_container* voiceControlContainer, HostControls hostConnect, UPlug* toPlug)
{
	assert((toPlug->flags & PF_HOST_CONTROL) != 0);
	assert(hostConnect == HC_VOICE_ACTIVE || hostConnect == HC_VOICE_VIRTUAL_VOICE_ID);

	UPlug* fromPin = nullptr;

	auto it2 = plugs.begin() + 2; // skip 2 MIDI plugs.
	for (auto& param : parameters_)
	{
		if (param.container == voiceControlContainer && param.hostConnect == hostConnect)
		{
			// Connect existing plug to module.
			fromPin = *it2;
			break;
		}
		++it2;
	}

	// else add a new one.
	if (fromPin == nullptr)
	{
		fromPin = new UPlug(this, DR_OUT, toPlug->DataType);
		// Keep track of what kind of plug (host-control or parameter).
		fromPin->SetFlag((toPlug->flags & (PF_HOST_CONTROL | PF_PATCH_STORE)) | PF_VALUE_CHANGED);

		AddPlug(fromPin);
		fromPin->CreateBuffer();

		// Attempt to automatically set modules polyphonic if they have poly host control inputs.
		// The exception is Voice/Active which does not cause a module to be polyphonic, but to monitor if it is. e.g. Scope3.
		assert(HostControlisPolyphonic(hostConnect) && toPlug->Direction == DR_IN && toPlug->DataType != DT_MIDI);
		if (hostConnect != HC_VOICE_ACTIVE)
		{
			fromPin->SetFlag(PF_POLYPHONIC_SOURCE);
		}

		parameters_.push_back({ nullptr, voiceControlContainer, hostConnect });

		voiceControlContainer->ConnectVoiceHostControl(hostConnect, fromPin);
	}

	connect_oversampler_safe(fromPin, toPlug);
}

void ug_patch_param_setter::ConnectHostParameter(dsp_patch_parameter_base* parameter, UPlug* toPlug)
{
	assert(parameter->getHostControlId() != HC_SNAP_MODULATION__DEPRECATED); // caller to handle.
	assert(parameter->getHostControlId() != HC_VOICE_ACTIVE); // caller to handle.

	// Find existing plug if already catered for. (more likely on host controls).
	auto it2 = plugs.begin() + 2; // skip 2 MIDI plugs.
	for (auto& param : parameters_)
	{
		if (param.parameter == parameter)
		{
			// Connect existing plug to module.
			connect_oversampler_safe(*it2, toPlug);
			return;
			break;
		}

		++it2;
	}

	// if no existing output pin found, create one.
	ConnectParameter(parameter, toPlug);
}

void ug_patch_param_setter::ConnectParameter(int32_t moduleHandle, int moduleParameterId, UPlug* plug)
{
	auto parameter = parent_container->get_patch_manager()->GetParameter(moduleHandle, moduleParameterId);
	ConnectParameter(parameter, plug);
}

void ug_patch_param_setter::ConnectParameter(dsp_patch_parameter_base* parameter, UPlug* toPlug)
{
	auto fromPin = new UPlug(this, DR_OUT, toPlug->DataType);
	// Keep track of what kind of plug (host-control or parameter).
	fromPin->SetFlag((toPlug->flags & (PF_HOST_CONTROL | PF_PATCH_STORE)));

	AddPlug(fromPin);
	fromPin->CreateBuffer();

	// Attempt to automatically set modules polyphonic if they have poly host control inputs.
	// The exception is Voice/Active which does not cause a module to be polyphonic, but to monitor if it is. e.g. Scope3.
	if (HostControlisPolyphonic(parameter->getHostControlId()) && toPlug->Direction == DR_IN && toPlug->DataType != DT_MIDI)
	{
		fromPin->SetFlag(PF_POLYPHONIC_SOURCE | PF_VALUE_CHANGED);
		if (HC_VOICE_PITCH == parameter->getHostControlId())
		{
			fromPin->SetBufferValue("5"); // Middle-A
		}
	}

	parameters_.push_back({ parameter, nullptr , HC_NONE });

	connect_oversampler_safe(fromPin, toPlug);
	parameter->ConnectPin2(fromPin);
}

void ug_patch_param_setter::TransferConnectiontoSecondary(UPlug* from)
{
	auto secondary = parent_container->GetParameterSetterSecondary();
	if(this == secondary)
	{
		return;
	}

#if 0 //def _DEBUG

	{
		_RPTN(_CRT_WARN, "TransferConnectiontoSecondary %d pin %d -> %d\n", Handle(), from->getPlugIndex(), secondary->Handle());
	}

#endif


	auto it2 = plugs.begin();
	++it2; // skip MIDI plugs.
	++it2;
	for (auto it = parameters_.begin(); it != parameters_.end(); ++it)
	{
		UPlug* plug = *it2;

		if( plug == from )
		{
			secondary->parameters_.push_back(*it);
			plug->setIndex(static_cast<int>(secondary->plugs.size()));
			plug->setUniqueId(static_cast<int>(secondary->plugs.size()));
			secondary->plugs.push_back(plug);
			plug->UG = secondary;

			plug->SetFlag(PF_PARAMETER_1_BLOCK_LATENCY); // see also UGF_UPSTREAM_PARAMETER

			it = parameters_.erase(it);
			it2 = plugs.erase(it2);
			break;
		}
		++it2;
	}

	// Renumber remaining plugs index to fill the gap.
	int i = 0;
	for (auto p : plugs)
	{
		p->setIndex(i);
		p->setUniqueId(i);
		++i;
	}
}

ug_base* ug_patch_param_setter::Clone( CUGLookupList& UGLookupList )
{
	auto clone = (ug_patch_param_setter*) ug_base::Clone( UGLookupList );

	// Copy list of parameters.
	auto it2 = clone->plugs.begin() + 2; // skip 2 MIDI plugs.
	for (auto& param : parameters_)
	{
		auto fromPin = *it2++;
		if (param.parameter)
		{
			param.parameter->ConnectPin2(fromPin);
		}
		else
		{
			param.container->ConnectVoiceHostControl(param.hostConnect, fromPin);
		}
	}

	return clone;
}

void ug_patch_param_setter::ReRoutePlugs()
{
	// Ensure ug_patch_param_setter always down-stream of ug_patch_automator by
	// connecting a dummy MIDI connection.
	
	assert(GetPlug(L"MIDI In") == GetPlug(0));
	parent_container->RouteDummyPinToPatchAutomator(GetPlug(0));
}
