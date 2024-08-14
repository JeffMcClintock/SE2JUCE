#include "pch.h"
#include "ug_voice_monitor.h"
#include "module_register.h"
#include "ug_container.h"
#include "ug_patch_param_setter.h"
#include "module_info.h"

SE_DECLARE_INIT_STATIC_FILE(ug_voice_monitor)

using namespace std;

#ifdef _DEBUG
// See also DEBUG_VOICE_ALLOCATION, DBF_TRACE_POLY
//#define DEBUG_VOICEWATCHER
#endif

namespace
{
REGISTER_MODULE_1( L"VoiceMonitor", 0, IDS_MG_DEBUG, ug_voice_monitor, 0, L"");
}

ug_voice_monitor::ug_voice_monitor() :
	m_voice_parameter_setter(0),
	voicePeakLevel(0)
{
}

void ug_voice_monitor::Resume()
{
	updateVoicePeakCount = peakCheckBlockPeriod;
	ug_base::Resume();
}

void ug_voice_monitor::DoProcess(int buffer_offset, int sampleframes)
{
	assert(sampleframes > 0);	//otherwise suspend probs
	ug_base::DoProcess(buffer_offset, sampleframes);

	if (current_out_state != ST_STOP)
	{
		// Calculate voice output peak level.
		auto it = plugs.begin();
		++it; // skip first plug (it's an output).

		for (; it != plugs.end(); ++it)
		{
			auto p = *it;

			if (p->getState() == ST_RUN)
			{
				float* i = p->GetSamplePtr() + buffer_offset;
				for (int s = sampleframes; s > 0; --s)
				{
					float v = *i++;
					if (v > voicePeakLevel)
						voicePeakLevel = v;
					if (-v > voicePeakLevel)
						voicePeakLevel = -v;
				}
			}
		}
	}

	if (AudioMaster()->BlockSize() == buffer_offset + sampleframes)	// End of block?
	{
		if (current_out_state_sent != current_out_state)
		{
			current_out_state_sent = current_out_state;
			
			ParentContainer()->GetParameterSetter()->AddEvent(
				new_SynthEditEvent(AudioMaster()->NextGlobalStartClock(), UET_VOICE_STREAMING_STATE, current_out_state, 0, pp_voice_num, 0));
		}
		
		// output has been silent long enough to shut down voice (so long as it's not held etc)
		if (ST_RUN != current_out_state && ++silentBlocks == 2)
		{
			ParentContainer()->GetParameterSetter()->AddEvent(
				new_SynthEditEvent(AudioMaster()->NextGlobalStartClock(), UET_VOICE_DONE_CHECK, current_out_state, 0, pp_voice_num, 0));
		}

		if (updateVoicePeakCount-- <= 0)
		{
			const int32_t peakLevelCastAsInt = *((int*)&voicePeakLevel); // crude method to pass float in int parameter.

			ParentContainer()->GetParameterSetter()->AddEvent(
				new_SynthEditEvent(AudioMaster()->NextGlobalStartClock(), UET_VOICE_LEVEL, current_out_state, peakLevelCastAsInt, pp_voice_num, 0));

			voicePeakLevel = 0;
			updateVoicePeakCount = peakCheckBlockPeriod;
		}
	}
}

void ug_voice_monitor::HandleEvent(SynthEditEvent* e)
{
	// send event to ug
	switch(e->eventType)
	{
	case UET_EVENT_STREAMING_START:
	case UET_EVENT_STREAMING_STOP:
		{
			UPlug* to_plug = GetPlug(e->parm1);

			const bool isStreaming = e->eventType == UET_EVENT_STREAMING_START;
			to_plug->setState(isStreaming ? ST_RUN : ST_STOP);
			
			if(isStreaming)
			{
				current_out_state = ST_RUN;
			}
			else
			{
				InputSetup();
			}

			silentBlocks = 0;
			
#ifdef DEBUG_VOICEWATCHER
			{
				auto	fromUg = to_plug->connections.front()->UG;
				int		voice = fromUg->pp_voice_num;
				if(isStreaming)
				{
					_RPTW3(_CRT_WARN, L"ug_voice_monitor voice %d pin %d RUN (from %s)\n", voice, to_plug->getPlugIndex(), DebugPrintMonitoredModules(to_plug->connections.front()).c_str());
				}
				else
				{
					_RPT2(_CRT_WARN, "ug_voice_monitor voice %d pin %d STOP\n", voice, to_plug->getPlugIndex());
				}
			}
#endif
			assert(to_plug->DataType == DT_FSAMPLE || !isStreaming); // ST_RUN only supported from streaming samples
		}
		break;

	default:
		ug_adder2::HandleEvent(e);
	};
}

#if defined( _DEBUG )
wstring ug_voice_monitor::DebugPrintMonitoredModules( UPlug* fromPlug ) // ug_base* fromUg )
{
	ug_base* fromUg = fromPlug->UG;
	wstring fromModules;
	wstring toModules;

	if( fromUg->getModuleType()->UniqueId() == L"SE Voice Mute" )
	{
		fromPlug = fromUg->plugs[0]->connections.front();
		fromUg = fromPlug->UG;

		if( dynamic_cast<ug_adder2*>(fromUg) )
		{
			fromModules = L"], ";
			for( size_t i = 1 ; i < fromUg->plugs.size() ; i++ )
			{
				if( fromUg->plugs[i]->getState() == ST_RUN )
				{
					ug_base* fromUg2 = fromUg->plugs[i]->connections.front()->UG;

					if( i > 1 )
						fromModules = L", " + fromModules;

					fromModules = fromUg2->DebugModuleName(true) + fromModules;
				}
			}
			fromModules = L"[" + fromModules;
		}
		else
		{
			fromModules = fromUg->DebugModuleName(true);
		}
	}
	else
	{
		fromModules = fromUg->DebugModuleName(true);
	}

	{
		for( auto to : fromPlug->connections )
		{
			if( !toModules.empty() )
			{
				toModules += L" -> ";
			}

			wstring n;

			// Prevent Voice-Mute's over-long description of it's inputs confusing the output.
			if( to->UG->getModuleType()->UniqueId() == L"SE Voice Mute" )
			{
				// Voice Mute always connected to a ug_voice_monitor and an adder.

				n = L"WTF!!";
				auto VoiceMuteOutputPin = to->UG->GetPlug(2); // Output pin.
				assert(VoiceMuteOutputPin->Direction == DR_OUT);

				for (auto topin : VoiceMuteOutputPin->connections)
				{
					// Look for voice-summing adder.
					if (topin->UG->GetPolyAgregate())
					{
						auto VoiceSummerOutputPin = topin->UG->GetPlug(0); // Output pin.
						n = L"[VM][ADD]->";
						if(!VoiceSummerOutputPin->connections.empty())
							n += VoiceSummerOutputPin->connections[0]->UG->DebugModuleName(true);
					}
				}
			}
			else
			{
				// DOn't bother tracing to Polyphonic modules, they are never responsible for holding a voice open.
				if( to->UG->GetPolyphonic())
					n = to->UG->DebugModuleName();
			}
			toModules += n;
		}
	}

	// Print all modules conencted to voice-mute, followed by all module they connect to (including voice mute).
	wstring res = L"{" + fromModules + L"} Connected To {" + toModules + L"}";
	return res;
}
#endif

void ug_voice_monitor::InputSetup()
{
	// Determine which input are running/stopped
	current_out_state = ST_STOP;

	auto it = plugs.begin();
	++it;						// skip first plug (it's an output).
	for(; it != plugs.end(); ++it)
	{
		UPlug*	p = *it;
		if(p->getState() == ST_RUN)
		{
			current_out_state = ST_RUN;
#ifdef DEBUG_VOICEWATCHER
			{
//				connector*	c = p->connectors.front();
				auto fromPlug = p->connections.front();
				auto fromUg = fromPlug->UG;
				int			voice = fromUg->pp_voice_num;
				_RPTW3(_CRT_WARN, L"ug_voice_monitor voice %d pin %d HELDOPEN (by %s)\n", voice, p->getPlugIndex(), DebugPrintMonitoredModules(fromPlug).c_str());
			}
#endif
			return;
		}
	}
}

// overide base class, deliberatly do nothing
int ug_voice_monitor::Open()
{
	ug_adder2::Open();
	SET_CUR_FUNC(&ug_base::process_nothing);

	// Synth voices will call open() during note-on
	// Drums call open on all voices before any note on.
	// so we need to suspend these voices immediately. (we wait 3 sample for things to settle a bit)
	current_out_state = ST_RUN;		// must assume voice running at startup

	m_voice_parameter_setter = ParentContainer()->GetParameterSetter();

#if defined(_DEBUG)
	assert(!GetPlug(0)->InUse());	// i.e. no need to send out stat.
	// to satisfy debug checks.
	OutputChange(SampleClock(), GetPlug(0), ST_STATIC);
#endif

	// send peak level estimate periodically to voice control.
	peakCheckBlockPeriod = static_cast<int>(getSampleRate() / ( 100 * AudioMaster()->BlockSize()));
	peakCheckBlockPeriod = (std::max)(1, peakCheckBlockPeriod);
	updateVoicePeakCount = peakCheckBlockPeriod;

	return 0;
}
