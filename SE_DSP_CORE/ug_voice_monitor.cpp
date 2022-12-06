// ug_voice_monitor Class
#include "pch.h"
#include "ug_voice_monitor.h"
#include "ug_container.h"

#include <assert.h>
#include "SeAudioMaster.h"
#include "module_register.h"
#include "./ug_event.h"
#include "iseshelldsp.h"
#include "ug_patch_param_setter.h"
#if defined( _DEBUG )
#include "ug_adder2.h"
#endif

SE_DECLARE_INIT_STATIC_FILE(ug_voice_monitor)

using namespace std;

/*
TODO: Flaws in this are that it checks voice members directly, even though current voice status may represent 'future' state (later in the same block).
FIX: this module should NEVER controls voice suspend/mute/wake, it can't know what the voice-allocator is doing exactly. info always arrives up to 1 block late.
      Instead it should merely report back (via event) to voice whenever voice goes silent. Voice can then consistently control allocation/muting etc, and ignore out-of-date reports.
*/

#ifdef _DEBUG
// See also DEBUG_VOICE_ALLOCATION, DBF_TRACE_POLY
//#define DEBUG_VOICEWATCHER
#endif

namespace
{
REGISTER_MODULE_1( L"VoiceMonitor", 0, IDS_MG_DEBUG, ug_voice_monitor, CF_HIDDEN, L"");
}

ug_voice_monitor::ug_voice_monitor() :
	current_out_state(ST_STOP),
	state_(VM_VOICE_OPEN),
	m_voice_parameter_setter(0),
	voicePeakLevel(0)
{
}

void ug_voice_monitor::Resume()
{
	updateVoicePeakCount = peakCheckBlockPeriod;
	state_ = VM_VOICE_OPEN;
	ug_base::Resume();
}

void ug_voice_monitor::DoProcess(int buffer_offset, int sampleframes)
{
	assert(sampleframes > 0);	//otherwise suspend probs
	ug_base::DoProcess(buffer_offset, sampleframes);

	if (current_out_state == ST_STOP)
	{
		static_input_count -= sampleframes;
	}
	else
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
		if (updateVoicePeakCount-- <= 0 || current_out_state != current_out_state_sent)
		{
			int peakLevelCastAsInt = *((int*)&voicePeakLevel); // crude method to pass float in int parameter.

			ParentContainer()->GetParameterSetter()->AddEvent(
				new_SynthEditEvent(AudioMaster()->NextGlobalStartClock(), UET_VOICE_OUTPUT_STATUS, current_out_state, peakLevelCastAsInt, m_voice->m_voice_number, 0));

			current_out_state_sent = current_out_state;
			voicePeakLevel = 0;
			updateVoicePeakCount = peakCheckBlockPeriod;
		}
	}

	// we have now processed entire block. Except in Fruityloops (which sub-divides block), hence blocksize check.
	// if input is stopped at this point, we can suspend voice
	if( current_out_state == ST_STOP )
	{
		if( AudioMaster()->BlockSize() == buffer_offset + sampleframes )	// not ok to suspend on sub-block.
		{

			// All inputs stopped. Is note in decay stage?
			// Note: It's not quite correct to check voice-state or hold pedal directly becuase it's a timestamped continuous value.
			bool noteHeld = (ParentContainer()->HoldPedalState() && m_voice->voiceActive_ == 1.0f ) && m_voice->NoteOffTime != 0; // ignore hold pedal at startup. We need to shut off voices regardless.
			const int midiCvGateDelay = 3;
			if( m_voice->NoteOffTime < SampleClock() - (timestamp_t) midiCvGateDelay /* && !noteHeld */ )	// Sounding notes have off time set to SE_TIMESTAMP_MAX, which wraps to a negative number if we dare add gate-delay. hence midiCvGateDelay on right-side.
			{
				if( state_ == VM_VOICE_OPEN )
				{
					if( !noteHeld || m_voice->voiceActive_ <= 0.0f ) // If hold-pedal down, keep voice active. Unless voice stolen (voiceactive <= 0).
					{
						// Deactivate sets voice-active to zero (and voice-state to MUTING),
						// this in turn will signal Voice-Mute to silence the voice.
#ifdef DEBUG_VOICEWATCHER
						_RPT4(_CRT_WARN, "ug_voice_monitor(%x) deactivate msg to Voice %d (%x) at time %d\n", this, pp_voice_num, m_voice, SampleClock());
#endif
						ParentContainer()->GetParameterSetter()->AddEvent(new_SynthEditEvent(AudioMaster()->NextGlobalStartClock(), UET_DEACTIVATE_VOICE, 0, 0, m_voice->m_voice_number, 0));

						state_ = VM_VOICE_MUTING;
					}
				}
				else
				{
					if( state_ == VM_VOICE_MUTING )
					{
						// a new note may just have started, but this's input may not have registered it yet
						// so don't suspend if theres a chance the note hasn't quite died.
						// seems to happen most with very short note durations.
						if( static_input_count < 0 /* && m_voice->NoteOffTime < (int) Sample Clock() */ )
						{
							// can't suspend itself (while executing),
							// so add event to notesource to suspend voice at start of next block.
							assert(m_voice->NoteOffTime != SE_TIMESTAMP_MAX);				// SE_TIMESTAMP_MAX indicates note held

							// can't suspend itself (while executing), so add event to notesource to suspend voice at start of next block
		#ifdef DEBUG_VOICEWATCHER
							_RPT4
							(
								_CRT_WARN,
								"ug_voice_monitor(%x) suspend msg to Voice %d (%x) at time %d\n",
								this,
								pp_voice_num,
								m_voice,
								SampleClock()
							);
		#endif

							// No. Keyboard2 can not do suspend, else by suspending itself while executing, it invalidates audiomaster's iterator in SeAudioMaster::sub_process().
							// try patch param setter instead. Assuming it will not suspend itself, and that it always executes first before any module in the voice.
							m_voice_parameter_setter->AddEvent(new_SynthEditEvent(SampleClock(), UET_SUSPEND, 0, 0, m_voice->m_voice_number, 0));
							state_ = VM_VOICE_SUSPENDING;
							suspendMessageNoteOffTime = m_voice->NoteOffTime;
						}
					}
					else
					{
						if( state_ == VM_VOICE_SUSPENDING )
						{
							// Unusual situation. Note has resumed while we were waiting for it to end, but we missed it. (very fast notes).
							if( suspendMessageNoteOffTime != m_voice->NoteOffTime )
							{
								state_ = VM_VOICE_OPEN; // quit waiting for note to suspend.
#ifdef DEBUG_VOICEWATCHER
								_RPT4(_CRT_WARN, "ug_voice_monitor(%x) gave up waiting for suspend Voice %d (%x) at time %d\n", this, pp_voice_num, m_voice, SampleClock());
#endif
							}
						}
					}
				}
			}
			else
			{
				// User may hit a key while voice is muting or suspending.
				// if so ensure state is reset to 'OPEN'.
				state_ = VM_VOICE_OPEN;
			}
		}
	}
	else
	{
		// Next time audio starts after suspend, assume voice open again.
		if( state_ == VM_VOICE_SUSPENDING )
		{
			state_ = VM_VOICE_OPEN;
		}
	}

	// this in turn will signal Voice-Mute to silence the voice.
#ifdef DEBUG_VOICEWATCHER

	if (reportTimer-- < 0)
	{
		reportTimer = 1000;

		_RPT3(_CRT_WARN, "ug_voice_monitor(%x) Voice %d (%x)", this, pp_voice_num, m_voice);
		switch (state_)
		{
		case VM_VOICE_OPEN:
			_RPT0(_CRT_WARN, "VM_VOICE_OPEN\n");
			break;
		case VM_VOICE_SUSPENDING:
			_RPT0(_CRT_WARN, "VM_VOICE_SUSPENDING\n");
			break;
		case VM_VOICE_MUTING:
			_RPT0(_CRT_WARN, "VM_VOICE_MUTING\n");
			break;
		}
	}
#endif
}

void ug_voice_monitor::HandleEvent(SynthEditEvent* e)
{
	// send event to ug
	switch(e->eventType)
	{
	case UET_EVENT_STREAMING_START:
	case UET_EVENT_STREAMING_STOP:
		{
			UPlug*		to_plug = GetPlug(e->parm1);
			state_type	new_state = ST_STOP;
			if(e->eventType == UET_EVENT_STREAMING_START)
			{
				new_state = ST_RUN;
			}

#ifdef DEBUG_VOICEWATCHER
			{
//				connector*	c = to_plug->connectors.front();
				auto	fromUg = to_plug->connections.front()->UG;
				int		voice = fromUg->pp_voice_num;
				if(new_state == ST_RUN)
				{
					_RPTW3(_CRT_WARN, L"ug_voice_monitor voice %d pin %d RUN (from %s)\n", voice, to_plug->getPlugIndex(), DebugPrintMonitoredModules(to_plug->connections.front()).c_str());
				}
				else
				{
					_RPT2(_CRT_WARN, "ug_voice_monitor voice %d pin %d STOP\n", voice, to_plug->getPlugIndex());
				}
			}
#endif
			assert(to_plug->DataType == DT_FSAMPLE || new_state != ST_RUN); // ST_RUN only supported from streaming samples
			to_plug->setState(new_state);
			InputSetup();
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
	static_input_count = AudioMaster()->BlockSize();

	// Determin which input are running/stopped
	current_out_state = ST_STOP;

	vector<UPlug *>::iterator	it = plugs.begin();
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

// clone needs pointer to it's voice
//ug_base* ug_voice_monitor::Clone(CUGLookupList& UGLookupList)
//{
//	auto new_voice = parent_container->back();
//	auto clone = (ug_voice_monitor*) ug_adder2::Clone(UGLookupList);
//	clone->SetVoice(new_voice);
//	return clone;
//}

// overide base class, deliberatly do nothing
int ug_voice_monitor::Open()
{
	ug_adder2::Open();
	SET_CUR_FUNC(&ug_base::process_nothing);

	m_voice = parent_container->at(pp_voice_num);

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

	return 0;
}
