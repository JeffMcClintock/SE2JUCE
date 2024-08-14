// VoiceList.cpp
// Class to help VoiceList keep track of cloned VList

#include "pch.h"
#include <assert.h>
#include <math.h>
#include "CVoiceList.h"
#include "ug_base.h"
#include "SeAudioMaster.h"
#include "ug_container.h"
#include "modules/shared/voice_allocation_modes.h"
#include "ug_oversampler.h"
#include "ug_voice_monitor.h"
#include "UgDatabase.h"
#include "ug_patch_automator.h"
#include "midi_defs.h"
#include "dsp_patch_manager.h"
#include "ug_patch_param_setter.h"
#include "SeException.h"

#ifdef DEBUG_VOICE_ALLOCATION
#include <iomanip>
#include <sstream>
#endif


using namespace std;


// Lookup a UGs clone, if not found return NULL
// this is because some UGs aren't cloned for polyphony
// e.g. wave_out
ug_base* CUGLookupList::LookupPolyphonic( ug_base* originalModule )
{
	auto it = find( originalModule );

	if( it != end() )
	{
		return (*it).second;
	}

	if( !originalModule->GetPolyphonic() )
	{
		return originalModule;
	}

	return nullptr; // not found, ok in case of feedback modules
}

ug_base* CUGLookupList::Lookup2( ug_base* originalModule )
{
	auto it = find( originalModule );

	if( it != end() )
	{
		return (*it).second;
	}

	return originalModule;
}

Voice::Voice(ug_container* container, int p_voice_number) :
	NoteNum(-1) // places all notes initialy in 'refresh' mode. Which allows them to be allocated immediatly.
	,NoteOffTime(-1) // help voices be allocated immediately
	,NoteOnTime(0)
	,open(false)
#if defined( _DEBUG )
	,suspend_flag(false)
#endif
	,m_voice_number(p_voice_number)
	,m_audiomaster(0)
	,voiceState_(VS_ACTIVE)
	,voiceActive_(0.0f)
	, container_(container)
{
}

Voice::~Voice()
{
	for (auto u : UGClones)
	{
		delete u;
	}
}

void Voice::refresh()
{
	if (IsSuspended())
	{
		Resume();
		NoteNum = -1; // 19/4/2018 prevent new note stealing this voice (thinking it's playing same note), because there may be a *real* voice playing this note that *won't* get stolen properly, and will linger.
		voiceState_ = VS_ACTIVE;
	}
}

void Voice::activate( timestamp_t p_clock/*, int channel*/, int voiceId )
{
	//_RPT1(_CRT_WARN, "V%d NoteOn()\n", m_voice_number );
	NoteNum		= static_cast<short>(voiceId);
	NoteOnTime = p_clock;
	NoteOffTime	= SE_TIMESTAMP_MAX; // very important. Lets voicemonitor know note is active
	//	Channel		= channel;
	//last_note_held = false;
	assert( open ); // should now always be opened when created
	assert( suspend_flag == (voiceState_ == VS_SUSPENDED) );

	if( IsSuspended() ) //voiceState_ == VS_ SUSPENDED )
	{
		Resume();
	}

	voiceState_ = VS_ACTIVE;
	//	_RPT1(_CRT_WARN, "V%d voiceState_ = VS_ACTIVE\n", m_voice_number );
}

void Voice::deactivate(timestamp_t timestamp) // entering release phase
{
	NoteOffTime = timestamp; // use time from master notesource
}

void Voice::reassignVoiceId( short noteNumber )
{
	NoteNum = noteNumber;
}

void Voice::NoteOff( timestamp_t p_clock )
{
	//_RPT1(_CRT_WARN, "V%d NoteOff()\n", m_voice_number );
	deactivate(p_clock);

//	MessageNote Source( p_clock, UET_NOTE_OFF, NoteNum );
}

void Voice::NoteMute( timestamp_t p_clock )
{
	voiceState_ = VS_MUTING;

	//	_RPT2(_CRT_WARN, "V%d voiceState_ = VS_MUTING ts=%d\n", m_voice_number, p_clock );
	NoteOffTime = p_clock; // use time from master notesource
}

// Temp shut down note generation to save CPU
void Voice::Suspend2( timestamp_t /*p_clock*/ )
{
	assert( open );

	// happens when voice sustain set at 0, voice monitor thinks note has shut off
	// but we can't suspend the voice until note off msg received ( causes lost notes)
	if( NoteOffTime == SE_TIMESTAMP_MAX || IsSuspended() )
	{
		_RPT1(_CRT_WARN, "V%d voiceState_ = NOT-SUSPENDED\n", m_voice_number);
		return;
	}

	assert( suspend_flag == false );

	// Why send event when closing voice, they will never arive?
	voiceActive_ = -5.0f; // suspended.
	/*
		// Alesis drum machine generates note-off imediately after note on
		// can cause voice to suspend on note-on
		assert( NoteOnTime != Note OffTime || Note OnTime == 0 );
	*/
	//DEBUG_TIMESTAMP;
	//	_RPT2(_CRT_WARN, "Suspend, Voice %x, clock %d\n", this, p_clock );
	// suspend all poly ugs.
	for (auto u : UGClones)
	{
		assert(u->GetPolyphonic());
		{
			u->Suspend();
		}
	}

#if defined( _DEBUG )
	suspend_flag = true;
#endif
	voiceState_ = VS_SUSPENDED;

	//_RPT1(_CRT_WARN, "TS %d\n", p_clock );
	//_RPT1(_CRT_WARN, "V%d voiceState_ = VS_SUSPENDED\n", m_voice_number );
	// Give all dead notes equal chance of allocation
	// to prevent recycling keepalive.
	NoteOffTime = 0;
	// OLD, not true. --- ?NOTE: OnVoiceSuspended() below might reallocate this voice, so don't mess with any members below this.
//	NoteSource->parent_container->OnVoiceSuspended(p_clock);
}

void Voice::Resume()
{
	assert( open );

	//DEBUG_TIMESTAMP;
	//_RPT2(_CRT_WARN, "Resume voice (%d), Note %d\n", m_voice_number, NoteNum);
	// Re-Open polyphonic ugs, except notesource
	for (auto u : UGClones)
	{
		assert(u->GetPolyphonic()); // only need to re-open polyphonic ones

		u->Resume();
	}

#if defined( _DEBUG )
	suspend_flag = false;
#endif
	//		_RPT1(_CRT_WARN, "Note On un-Suspend, Voice %x\n", this );
}

ug_base* Voice::AddUG( ug_base* ug )
{
	UGClones.push_back(ug);

	// Note any UNotesource objects added to this voice
	if( ug->GetPolyGen() )
	{
		if( SetNoteSource( ug ) )
		{
			throw(SeException(SE_MULTIPLE_NOTESOURCES));
		}
	}
	if (ug->GetFlag(UGF_PATCH_AUTO))
	{
		assert(ug->ParentContainer()->automation_input_device == nullptr);
		ug->ParentContainer()->automation_input_device = dynamic_cast<ug_patch_automator*>(ug);
	}

	return ug;
}

void Voice::RemoveUG(ug_base* ug)
{
	assert(std::find(UGClones.begin(), UGClones.end(), ug) != UGClones.end());
	UGClones.erase(std::find(UGClones.begin(), UGClones.end(), ug));
}

/////////////////////  VOICELIST ////////////////////////
VoiceList::VoiceList( ) :
	Polyphony(8)
	,monoNotePlaying_(-1)
	,note_memory_idx(-1)
	,nextCyclicVoice_(0)
	,benchWarmerVoiceCount_(3)
	,holdPedalState_(false)
	,is_polyphonic(false)
	, is_polyphonic_cloned(false)
	, is_polyphonic_delayed_gate(false)
	, voiceAllocationMode_(-1)
	, overridingVoiceAllocationMode_(-1)
	, keysHeldCount(0)
	, mrnPitch(-1000.0f) // unlikely initial value.
	, mrnTimeStamp(0)
	, portamento_(0)
{
	for (int i = 0; i < maxVoiceId; i++)
	{
		note_status[i] = false;
	}
	
	#if defined( DEBUG_VOICE_ALLOCATION )
	    // !!! can open 100's of log files !!!!
		loggingFile.open("c:\\temp\\log.txt");
		loggingFile << "SynthEdit" << std::endl;
	#endif
}

VoiceList::~VoiceList()
{
	for (auto voice : *this)
		delete voice;
}

void VoiceList::AddUG( ug_base* ug )
{
	back()->AddUG( ug );
	//	_RPT2(_CRT_WARN, "%s added to voice %d\n", ug->DebugModuleName(), voice );
}

void VoiceList::VoiceAllocationNoteOn(timestamp_t timestamp, /*int midiChannel,*/ int MidiKeyNumber/*, int velocity*/, int usePhysicalVoice)
{
	if (!note_status[MidiKeyNumber])
	{
		note_status[MidiKeyNumber] = true;
		++keysHeldCount;
	}

	note_memory_idx++;
	note_memory_idx %= MCV_NOTE_MEM_SIZE;
	note_memory[note_memory_idx] = static_cast<char>(MidiKeyNumber);

	// don't bother allocating overriden mono notes
	{
		const int lVoiceAllocationMode = overridingVoiceAllocationMode_ != -1 ? overridingVoiceAllocationMode_ : voiceAllocationMode_;
		const int notePriority = (lVoiceAllocationMode >> 8) & 0x03;
		const bool monoMode = voice_allocation::isMonoMode(lVoiceAllocationMode);

		if (monoMode && notePriority != NP_NONE)
		{
			switch (notePriority)
			{
			case NP_LOWEST:
				for (int i = 0; i < MidiKeyNumber; ++i)
				{
					// A lower note already held, no voice allocation needed.
					if (note_status[i])
					{						
						return;
					}
				}

				break;

			case NP_HIGHEST:
				for (int i = 127; i > MidiKeyNumber; --i)
				{
					// A higher note already held, no voice allocation needed.
					if (note_status[i])
					{
						return;
					}
				}
				break;
			};
		}
	}

	noteStack.push(timestamp, MidiKeyNumber, usePhysicalVoice);

	PlayWaitingNotes(timestamp);
}

void VoiceList::PlayWaitingNotes(timestamp_t timestamp)
{
	if(noteStack.empty())
		return;

#if defined( DEBUG_VOICE_ALLOCATION )
	_RPT0(0, "PlayWaitingNotes()\n");
#endif

	// we probably already stole a voice and we're waiting for it to mute. (this is the usual case).
	// so don't keep stealing more unless we have to.
	int voicesPotentiallyAvailable = 0;
	for (int i = 1; i < size(); ++i)
	{
		auto v = at(i);
		if(v->voiceState_ != VS_ACTIVE)
		{
			++voicesPotentiallyAvailable;
		};
	}

	while(!noteStack.empty())
	{
		// Usually the first failure to allocate will steal a voice, and we can grab it here.
		// However it's possible for an unrelated note to sneak in and grab the stolen voice, leavin me with nothing (so we gotta steal *another* voice).
		const bool no_voices_avail_do_some_stealing = voicesPotentiallyAvailable <= 0;

		if (AttemptNoteOn(timestamp, noteStack.top().midiKeyNumber, noteStack.top().usePhysicalVoice, noteStack.top().canSteal || no_voices_avail_do_some_stealing))
		{
			noteStack.pop();
			--voicesPotentiallyAvailable;
		}
		else
		{
			noteStack.top().canSteal = false;
			break;
		}
	}
}

// try to allocate a voice and play a note.
bool VoiceList::AttemptNoteOn(timestamp_t timestamp, int MidiKeyNumber, int usePhysicalVoice, bool steal)
{
	const int lVoiceAllocationMode = overridingVoiceAllocationMode_ != -1 ? overridingVoiceAllocationMode_ : voiceAllocationMode_;
	const bool monoMode = voice_allocation::isMonoMode(lVoiceAllocationMode);

	// Assign MIDI Chan/Key to Virtual Voice ID.
	assert(MidiKeyNumber >= 0 && MidiKeyNumber < maxVoiceId);
	const bool sendTrigger = !monoMode || voice_allocation::isMonoRetrigger(lVoiceAllocationMode) || monoNotePlaying_ == -1;

	Voice* voice{};

	if (usePhysicalVoice >= 0) // drum mode.
	{
		// When using drum-trigger, voice may not exist...
		// when output connected only to Volt-Meter AND annother output connected to same voltmeter.
		// drum trigger handles case when only one output conencted to meter OK.
		if (usePhysicalVoice < (int)size())
		{
			voice = at(usePhysicalVoice);
		}
		else
		{
			voice = back(); // hack to allow drum-trigger to send trigger signal. Might wake wrong voice.
		}
	}
	else
	{
		voice = allocateVoice(timestamp, /*channel,*/ MidiKeyNumber, lVoiceAllocationMode, steal);
	}

	if (!voice)
		return false;

	monoNotePlaying_ = MidiKeyNumber;

	const bool autoGlide = keysHeldCount > 1; // because we have 2 or more keys held

	DoNoteOn(timestamp, voice, MidiKeyNumber, sendTrigger, autoGlide);

	return true;
}

void Voice::DoneCheck(timestamp_t timeStamp)
{
	// already suspended?
	if (voiceState_ == VS_SUSPENDED)
		return;

	// output buffer not cleared?
	if (outputSilentSince > timeStamp - container_->AudioMaster()->BlockSize())
		return;

	// output is silent, and output buffer is full of silence.

	// note-off might be scheduled during this coming block. i.e. not quite yet.
	// Also catches notes which are held (except by sustain pedal)
	if(NoteOffTime >= timeStamp - container_->AudioMaster()->BlockSize()) // 2 blocks ago to account for PF_PARAMETER_1_BLOCK_LATENCY (a MIDI-CV upstream of a MIDI-CV which has signals delayed by 1 block)
		return;

	// is key held by sustain? don't shut off voice.
	if (voiceActive_ == 1.0f && container_->HoldPedalState())
	{
		return;
	}

	container_->suspendVoice(timeStamp, m_voice_number);
}

// MIDI-CV modules control the voices only in their own container. There may be several MIDI-CVs sharing a Patch Automator.
// This is different than KeyBoard2 which controls all voices in the Patch Automator's container.
// Since we can't use the Patch Automator, got to keep a list of param setters 'below' here.
/*
VoiceActive: 1.0 = note playing
             0.5 playing but only an secondary overlap (ignore hold-pedal)
			 0.0 = slow-steal (20ms)
			-1.0 = abrupt steal (5ms)
			-5.0 = Suspended
See VoiceMute module.
*/

void VoiceList::SetVoiceParameters( timestamp_t timestamp, Voice* voice, float voiceActive, int voiceId )
{
	if( voiceActive > -10.0f ) // no change
	{
		// prevent spurious signals (causes envelope to reset with mono-note-priority when releasing note while other held).
		bool voiceActiveChanged = voice->voiceActive_ != voiceActive;
		voice->voiceActive_ = voiceActive;

		if( voiceActiveChanged )
		{
#if defined( DEBUG_VOICE_ALLOCATION )
			// Voice refresh causes many spurious notifications.
//			 _RPTW3(_CRT_WARN, L"                                          V%d  ACTIVE %f  (ts %d))\n", voice->m_voice_number, voiceActive, timestamp );
#endif
			voiceActiveHc_.sendValue( timestamp, (ug_container*) this, voice->m_voice_number, sizeof(float), &voiceActive );
		}
	}

	if( voiceId > -1 ) // Only need set this at note-on.
	{
		voiceIdHc_.sendValue( timestamp, (ug_container*) this, voice->m_voice_number, sizeof(int32_t), &voiceId );
	}
}

void VoiceList::xNoteOff( timestamp_t p_clock, /*int midiChannel,*/ int MidiKeyNumber, float /*velocity*/, int voiceAllocationMode ) // bool mono_mode, int m_mono_note_priority )
{
	bool mono_mode = voice_allocation::isMonoMode(voiceAllocationMode);
	int m_mono_note_priority = 0xff & (voiceAllocationMode >> 8);

	int voiceId = MidiKeyNumber;
	// handle last-note priority
	if( note_status[voiceId] )
	{
		note_status[voiceId] = false;
		--keysHeldCount;
	}

	if( mono_mode != 0 && m_mono_note_priority > 0 )
	{
		int replacement_note = -1;

		switch(m_mono_note_priority)
		{
		case 1: // Lo
			for (int i = 0; i < maxVoiceId; i++) // find lowest note
			{
				if( note_status[i] )
				{
					if( i > voiceId )
						replacement_note = i; // only jump if note-off was current lowest

					break;
				}
			}

			break;

		case 2: // Hi
			for (int i = maxVoiceId-1; i >= 0; i--)
			{
				if( note_status[i] )
				{
					if( i < voiceId )
						replacement_note = i;

					break;
				}
			}

			break;

		case 3: // Last
			if( note_memory[note_memory_idx] == voiceId )
			{
				char last_note;

				do
				{
					note_memory_idx--;

					if( note_memory_idx < 0 )
						note_memory_idx = MCV_NOTE_MEM_SIZE - 1;

					last_note = note_memory[note_memory_idx];
					note_memory[note_memory_idx] = -1;

					if( last_note >= 0 && note_status[last_note] )
					{
						replacement_note = last_note;
						note_memory_idx--;
						break;
					}
				}
				while(last_note >= 0);
			}

			break;
		};

		if( replacement_note >= 0 )
		{
			VoiceAllocationNoteOn(p_clock, /*midiChannel,*/ replacement_note);
			return;
		}
	}

	//	_RPT0(_CRT_WARN, "NoteOff\n" );
	NoteOff( p_clock, /*chan,*/ static_cast<short>(voiceId) );
}

void VoiceList::xAllNotesOff( timestamp_t /*p_clock*/ )
{
	// TODO.
	/*
		Note: If the device's Hold Pedal controller is on,
		the notes aren't actually released until the Hold Pedal is turned off.
		See All Sound Off controller message for turning off the sound of these notes immediately.
	*/
	for (int i = 0; i < maxVoiceId; ++i) // find lowest note
	{
		note_status[i]  = false;
	}
}

void VoiceList::NoteOff(timestamp_t p_clock, /*short chan,*/ short note_num )
{
	if( note_num > -1 )
	{
		for (auto it = begin() + 1; it != end(); ++it)
		{
			auto v = *it;
			if( v->NoteNum == note_num && v->voiceState_ == VS_ACTIVE/*&& v->Channel == chan*/ )
			{
				if(v->NoteOffTime == SE_TIMESTAMP_MAX /*&& !v->IsHeld()*/ )
				{
					DoNoteOff(p_clock, v);
				}
			}
		}
	}
	else	// Special case for all notes off
	{
		for (auto it = begin() + 1; it != end(); ++it)
		{
			auto v = *it;
			const float voiceActive = 0.0f; // Note-mute fade out quickly.
			DoNoteOff( p_clock, v, voiceActive );
		}
	}
}

void Voice::CloneFrom(Voice* p_copy_from_voice)
{
	CUGLookupList UGLookupList;
	CloneUGsFrom( p_copy_from_voice, UGLookupList );
	CloneConnectorsFrom( p_copy_from_voice, UGLookupList );
}

void Voice::MoveFrom(Voice* FromVoice)
{
	// Copy each polyphonic UG
	for (auto it = FromVoice->UGClones.begin() ; it != FromVoice->UGClones.end() ; )
	{
		auto u = *it;

		if (u->GetPolyphonic())
		{
			it = FromVoice->UGClones.erase(it);
			AddUG(u);
		}
		else
		{
			++it;
		}
	}
}

void Voice::Open(ISeAudioMaster* p_audiomaster, bool setVoiceNumbers)
{
	assert( !open );
	m_audiomaster = p_audiomaster;
	//	_RPT1(_CRT_WARN, "Voice::Open %d\n", m_voice_number );
	//	_RPT1(_CRT_WARN, "Open, Voice %x\n", this );
	/*
		DEBUG_TIMESTAMP;
		if( NoteSource )
			_RPT1(_CRT_WARN, "Open, Note %d\n", NoteNum );
		else
			_RPT0(_CRT_WARN, "Open, new Voice\n" );
	*/
#ifdef _DEBUG
	int last_sortorder = -1;
#endif

	// Open all VList
	for (auto u : UGClones)
	{
		assert( (u->flags & UGF_OPEN) == 0 );

		// u->SetAudioMaster(p_audiomaster);
		assert(u->AudioMaster() == p_audiomaster); // check above line redundant.

		// Normally we set poly voice number on modules within each voice, unless we are an oversampling container,
		// in which case voice number will already be propagated down from parent container.
		if( setVoiceNumbers && u->GetPolyphonic() )
		{
			u->SetPPVoiceNumber( m_voice_number );
		}

		u->Open();

#ifdef _DEBUG
		assert(u->SortOrder2 > last_sortorder );
		last_sortorder = u->SortOrder2;
	#if _MSC_VER >= 1600 // Not Avail in VS2005.
		unsigned int fpState;
		_controlfp_s( &fpState, 0, 0 ); // flush-denormals-to-zero mode?
		assert( (fpState & _MCW_DN) == _DN_FLUSH && "err: flush denormals altered during Open()." );
	#endif
#endif
		assert( (u->flags & UGF_OPEN) != 0 ); // did ug's Open() call base_class::Open()???
	}

	if (!p_audiomaster->SilenceOptimisation())
	{
		for (auto u : UGClones)
		{
			u->AddEvent(p_audiomaster->AllocateMessage((std::numeric_limits<timestamp_t>::max)(), UET_FORCE_NEVER_SLEEP));
		}
	}

#if defined( _DEBUG )
	suspend_flag = false;
#endif
	open = true;
}

void Voice::SetPPVoiceNumber(int n)
{
	// ??? need to set m_voice_number?, will be zero in polyphonic oversampler containers. Since there's only one voice per oversampler.

	// Give all polyphonic modules a voice number. non-polyphonic are -1.
	for (auto u : UGClones)
	{
//		assert( u->pp_voice_num == m_voice_number || u->pp_voice_num == -1);

		u->SetPPVoiceNumber( n );
	}
}

// call Close() function for all VList
int Voice::Close()
{
	if( !open )
		return 0;

	for (auto u : UGClones)
	{
		assert( (u->flags & UGF_OPEN) != 0 );
		u->Close();
		assert( (u->flags & UGF_OPEN) == 0 ); // did ug's Close() call base class Close()???
	}

	open = false;
	return 0;
}

void VoiceList::OpenAll(ISeAudioMaster* p_audiomaster, bool setVoiceNumbers)
{
	const float maxHoldBackMs = 25; // reasonable time within which to trigger held-back note (else it's jarring to play it much too late).
	maxNoteHoldBackPeriodSamples = (int) (p_audiomaster->SampleRate() * maxHoldBackMs * 0.001f); //  see also: VoiceMute fadeOutTimeFastMs

	// V0 must be opened first (notesource assumes so)
	for (auto v : *this)
	{
#ifdef _DEBUG
//		v->CheckSortOrder();
#endif
		v->Open(p_audiomaster, setVoiceNumbers);
	}
}

void VoiceList::SortAll3()
{
	for (auto voice : *this)
		voice->Sort3();
}

void VoiceList::CloneVoices()
{
	// for synths, clone voices for polyphony.
	if (is_polyphonic_cloned)
	{
		// First voice we move modules out of voice 0.
		auto v1 = new Voice(static_cast<ug_container*>(this), (int)size());
		push_back(v1);			// create a new voice
		v1->MoveFrom(front());	// copy it's ugs from voice zero

		const int totalVoices = minimumPhysicalVoicesRequired();
		_RPTN(_CRT_WARN, "CloneVoices() total = %d\n", totalVoices);

		for (int p = totalVoices; p > 1; --p)
		{
			auto v = new Voice(static_cast<ug_container*>(this), (int) size());
			push_back(v);		// create a new voice
			v->CloneFrom(v1);	// copy it's ugs from voice zero
		}
	}
}

// Call Close() on all UGs for all Voices
void VoiceList::CloseAll()
{
	for (auto v : *this)
	{
		v->Close();
	}
}

void VoiceList::setVoiceCount(int c)
{
	// CK Sample System can only handle 128
	if (c > maxVoiceId)
	{
		c = maxVoiceId;
	}

	auto container = dynamic_cast<ug_container*>(this);

	if(Polyphony > c && container->GetFlag(UGF_OPEN)) // don't send all-notes-off during construction of graph
	{
		// Kill sounding notes so we don't get a polyphony 'overhang' (temporary excess voices playing)
		// Voice manager can only kill one voice per new note, so reducing polyphony appears to happen gradually otherwise.
		const auto ts = container->AudioMaster()->NextGlobalStartClock();
		NoteOff(ts, -1); // All notes Off
	}

	Polyphony = static_cast<short>(c);
}

void VoiceList::setVoiceReserveCount(int c)
{
	benchWarmerVoiceCount_ = (std::min)(32, c);
}

int VoiceList::voiceReserveCount() const
{
	return benchWarmerVoiceCount_;
}

int VoiceList::getPhysicalVoiceCount()
{
	return (int) size() - 1;
}

// clone A Voice (for polyphonic sound)
// not all UGs are cloned, eg sliders don't need to be
// passed a voice to copy (may be in same container, mayby not)
// also passed the 'map' of UGs--clones to add to (CUGLookupList)
void Voice::CloneUGsFrom( Voice* FromVoice, CUGLookupList& UGLookupList )
{
#ifdef VOICE_USE_VECTOR
	UGClones.reserve(FromVoice->UGClones.size());
#endif

	// Copy each polyphonic UG
	for (auto u : FromVoice->UGClones)
	{
		assert(u->GetPolyphonic());
		{
			auto clone = u->Clone( UGLookupList );

			if( clone != NULL)
			{
				AddUG( clone );
				clone->SetPPVoiceNumber(m_voice_number);
				UGLookupList.insert( std::pair<ug_base*,ug_base*>( u, clone ) );
			}
		}
	}
}

// this is called after CloneUGFrom to complete the connections
void Voice::CloneConnectorsFrom( Voice* FromVoice, CUGLookupList& UGLookupList )
{
	// Add connectors
	for (auto master_ug : FromVoice->UGClones)
	{
		// only nesc for cloned(polyphonic modules)
		assert(master_ug->GetPolyphonic());
		{
			auto clone_ug = UGLookupList.LookupPolyphonic( master_ug );
			clone_ug->CloneConnectorsFrom( master_ug, UGLookupList );
		}
	}
}

// copy a Voice (for oversampling),
// All UGs are cloned.
void Voice::CopyFrom(ISeAudioMaster* audiomaster, ug_container* parent_container, Voice* FromVoice, CUGLookupList& UGLookupList)
{
	CUGLookupList UGLookupList_local;

	// Copy each UG.
	for (auto u : FromVoice->UGClones)
	{
		auto clone = u->Copy(audiomaster, UGLookupList_local);
		clone->parent_container = parent_container;

#if 0 // helped in some ways, not in others
		// clone may normally have been monophonic, but since we're cloning the entire container, it MUST be polyphonic now.
		clone->SetPolyphonic(true);
		u->SetPolyphonic(true);
#endif
		assert( clone != NULL);
		AddUG( clone );
		UGLookupList_local.insert( std::pair<ug_base*,ug_base*>( u, clone ) );
	}

	// Copy Connectors.
	for( auto& it : UGLookupList_local )
	{
		it.second->CopyConnectorsFrom(it.first , UGLookupList_local);
	}

	UGLookupList.insert(UGLookupList_local.begin(), UGLookupList_local.end() );
}

// Find Voice index (playing or releasing), given MIDI note number
// TODO!! get rid of this, assumes only one voice playing a given note number.
Voice* VoiceList::GetVoice(/*short chan,*/ short MidiNoteNumber)
{
	Voice* bestReleasingVoice = nullptr;

//	_RPT0(_CRT_WARN, "Get Voice() ---------------\n");
	for (auto it = begin() + 1; it != end(); ++it)
	{
		auto v = *it;
		//		_RPT3(_CRT_WARN, "    voice %3d key=%d playing=%d\n", v->m_voice_number, v->NoteNum, (int)( v->NoteOffTime == SE_TIMESTAMP_MAX ));
		if( v->NoteNum == MidiNoteNumber /*&& v->Channel == chan*/ )
		{
			/* new. do need to send controllers to voice in release stage. Still may need to ignore stolen, fast-release, voices.
						//  Need to get playing note (not released one, or held-by-sustain).
						// This allows multiple notes on same midi channel to work (or at least not stick)
						// ( Soundfont Player will use a new voice to play same note while old note dies out. )
			*/

			if(v->NoteOffTime == SE_TIMESTAMP_MAX /* && !v->IsHeld() */)
			{
				return v;
			}
			else
			{
				bool better;
				if (bestReleasingVoice == nullptr)
				{
					better = true;
				}
				else
				{
					// A voice is a better candidate if it's active.
					if ((v->voiceState_ == VS_ACTIVE) != (bestReleasingVoice->voiceState_ == VS_ACTIVE))
					{
						better = (v->voiceState_ == VS_ACTIVE) > (bestReleasingVoice->voiceState_ == VS_ACTIVE);
					}
					else
					{
						better = v->NoteOffTime > 0 && v->NoteOffTime > bestReleasingVoice->NoteOffTime;
					}
				}
				if (better)
				{
					bestReleasingVoice = v;
				}
			}
		}
	}

	return bestReleasingVoice;
}

void VoiceList::ConnectVoiceHostControl(HostControls hostConnect, UPlug* plug)
{
	switch( hostConnect )
	{
		case HC_VOICE_ACTIVE:
			voiceActiveHc_.ConnectPin( plug );
		break;

		case HC_VOICE_VIRTUAL_VOICE_ID:
			voiceIdHc_.ConnectPin( plug );
		break;

		//case HC_VOICE_GATE:
		//	voiceGateHc_.ConnectPin( plug );
		//break;
        default:
            assert(false);
            break;
	}
}

bool VoiceList::OnVoiceControlChanged(HostControls hostConnect, int32_t /*size*/, const void* data)
{
	switch( hostConnect )
	{
	case HC_VOICE_ALLOCATION_MODE:
	{
		voiceAllocationMode_ = *(int32_t*)data;
	}
	break;

	case HC_PORTAMENTO:
	{
		portamento_ = *(float*)data;
	}
	break;

	default:
		break;
	}

	return false;
}

void Voice::IterateContainersDepthFirst(std::function<void(ug_container*)>& f)
{
	for (auto ug : UGClones)
	{
		ug->IterateContainersDepthFirst(f);
	}
}

// Setup ugs for polyphony
// A ug is polyphonic only if upstream is a Notesource (GetPolyGen() == true )
// AND it is poly-active, or a downstream one is.
void Voice::PolyphonicSetup()
{
#ifdef _DEBUG
	if( SeAudioMaster::GetDebugFlag( DBF_TRACE_POLY ) )
	{
		_RPT0(_CRT_WARN, "\n----------------------------------\n");
		_RPT0(_CRT_WARN, "STAGE 2 - Flag upstream modules\n");
	}
#endif

	if (UGClones.empty())
	{
		return;
	}

	{
		// try to avoid this when oversampler is already a polyphonic clone. tends to crash.
		auto oversampler = dynamic_cast<ug_oversampler*>(container_->AudioMaster());
		if (oversampler && oversampler->GetFlag(UGF_POLYPHONIC))
			return;
	}

	// Polyphonic setup for synths
	{
		// Find 'Active' UGs and propogate flag upstream
		for( auto ug : UGClones )
		{
			if( ug->PPGetActiveFlag() )
				ug->PPSetUpstream();  // Flag all upstream polyphonic UGs
		}

		if (container_->isContainerPolyphonic())
		{
			if (container_->isContainerPolyphonicCloned())
			{
				SetUpVoiceAdders();
			}

			SetUpNoteMonitor();

			// Periodic voice wakeup to prevent filter clicks after e.g. changing preset.
		}
	}

#ifdef _DEBUG

	if( SeAudioMaster::GetDebugFlag( DBF_TRACE_POLY ) )
	{
		_RPT0(_CRT_WARN, "\n----------------------------------\n" );
	}

#endif
}

void Voice::SetUpVoiceAdders()
{
	// synths need adders at end of each voice chain, unless they are monophonic
	// drum modules don't ( because voice is never cloned )
#ifdef VOICE_USE_VECTOR
	for (int i = 0 ; i < UGClones.size() ; ++i) // iterate by index because inserting a module invalidates iterators.
	{
		auto u = UGClones[i];
#else
	for( auto u : UGClones)
	{
#endif
		u->InsertVoiceAdders();
	}
}

void Voice::CloneContainerVoices()
{
	// connect each 'end-of-voice' ug to a UVoiceMonitor
	for (auto ug : UGClones)
	{
		ug->CloneContainerVoices();
	}
}

// NEW behavior: monitors ug if it has at least 1 connection to non-polyphonic module
// this works better in patch's with several outputs, (e.g osc->switch->waveshaper->out or osc->switch->out)
void Voice::SetUpNoteMonitor( bool secondPass )
{
	ug_voice_monitor* voice_mon = 0;

	// connect each 'end-of-voice' ug to a UVoiceMonitor
#ifdef VOICE_USE_VECTOR
	for (int i = 0; i < UGClones.size(); ++i) // iterate by index because inserting a module invalidates iterators.
	{
		auto u = UGClones[i];
#else
	for (auto u : UGClones)
	{
#endif
		if( u->GetPolyphonic() )
		{
			// iterate output plugs
			for( auto p : u->plugs)
			{
				if( p->Direction == DR_OUT && p->DataType == DT_FSAMPLE && p->InUse() )
				{
					bool monitor_this_plug = false;

					if( secondPass ) // No terminal module found, Try to find last audio connection in chain. Even if it's to a polyphonic module (Trigger-to-MIDI)
					{
						// Voice Monitor Fallback
						// having difficulty finding voices terminal module, try other strategies.
						for( auto to_plug : p->connections )
						{
							if( to_plug->UG->GetPolyphonic() )
							{
								if( to_plug->UG->GetFlag(UGF_VOICE_SPLITTER) )
								{
									monitor_this_plug = true;
									break;
								}

								bool hasAudioInButNotOut = false;

								// no audio outputs?
								for( auto p2 : to_plug->UG->plugs )
								{
									if( p2->Direction == DR_OUT && p2->DataType == DT_FSAMPLE && p2->InUse() )
									{
										hasAudioInButNotOut = true;
										break;
									}
								}

								if( !hasAudioInButNotOut ) // e.g. Trigger to MIDI
								{
									monitor_this_plug = true;
									break;
								}
							}
						}
					}
					else // first pass.
					{
						if(u->GetFlag(UGF_PP_TERMINATED_PATH)) // Ignore any Module leading to a 'hanging' path with no audio output.
						{
							for(auto to_plug : p->connections)
							{
								if(!to_plug->UG->GetPolyphonic() && !to_plug->UG->IgnoredByVoiceMonitor())
								{
									monitor_this_plug = true;
									break;
								}
							}
						}
					}

					if( monitor_this_plug )
					{
#ifdef _DEBUG
						u->SetFlag(UGF_DEBUG_POLY_LAST);
#endif

						// connect to voice monitor.
						if( voice_mon == 0)
						{
							auto voiceMonitorType = ModuleFactory()->GetById( (L"VoiceMonitor") );
							// Insert a UVoiceMonitor, it will suspend the voice once the note is done
							voice_mon = (ug_voice_monitor*) voiceMonitorType->BuildSynthOb();
							u->AudioMaster()->AssignTemporaryHandle(voice_mon);
							voice_mon->SetAudioMaster( u->AudioMaster() );
							voice_mon->AddFixedPlugs();
							voice_mon->SetupDynamicPlugs();
							container_->AddUG(this, voice_mon); // actually added to last voice (maybe should go to voice zero)
							// force cloning voicemonitors
							voice_mon->SetPolyphonic(true);
						}

						// now connect it to voice monitor
						// bit tricky, tail ug is already connected to adder
						// so need to temp reset p->Proxy to null so voicemonitor don't connect to adder
						UPlug* proxy = p->Proxy;
						p->Proxy = NULL;
						u->connect( p, voice_mon->plugs[1] );	// plug[1] is input
						// restore proxy
						p->Proxy = proxy;
#ifdef _DEBUG

						if( SeAudioMaster::GetDebugFlag( DBF_TRACE_POLY ) )
						{
							u->DebugPrintName(true);
							_RPT0(_CRT_WARN, "-Attached Voice Monitor\n" );
						}

#endif
					}
				}
			}
		}

#if defined(_WIN32) && defined(_DEBUG)

		if( voice_mon && voice_mon->plugs.empty() )
		{
			assert(false); // err: Voice Monitor has no inputs
		}
#endif
	}

	// in rare occasions voice can have zero audio outputs (like when using Trigger-to-MIDI).
	// in these case, monitor the last polyphonic module with audio outputs, possibly the MIDI-CV.
	if( voice_mon == 0 && !secondPass )
	{
		SetUpNoteMonitor( true );
	}
}

// returns true if another Notesouce is in this container or parent (not allowed, print error)
bool Voice::SetNoteSource(ug_base* m)
{
	container_->SetContainerPolyphonic();
	if ((m->GetFlags() & UGF_POLYPHONIC_GENERATOR_CLONED) != 0)
	{
		container_->SetContainerPolyphonicCloned();

		// only needed in editor
		{
			auto c = container_->parent_container;
			while (c)
			{
				if (c->isContainerPolyphonicCloned())
				{
					return true; // Another Notesouce is in this container (not allowed, print error).
				}

				c = c->parent_container;
			}
		}
	}
	if ((m->GetFlags() & UGF_DELAYED_GATE_NOTESOURCE) != 0)
	{
		container_->SetContainerPolyphonicDelayedGate();
	}

	return false;
}

void VoiceList::SendProgChange( ug_container* p_patch_control_container, int patch)
{
	//	front()->SendProgChange( p_original_container, patch ); // all controls are in voice zero(for synths), what about waveshaper?
	// Drum trigger moves each control to appropriate voice. Waveshapers etc will be spead over all voices
	for (auto v : *this)
	{
		v->SendProgChange( p_patch_control_container, patch );
	}
}

void Voice::SendProgChange( ug_container* p_patch_control_container, int patch )
{
	if( !open )
		return;

	if( ! UGClones.empty() )
	{
		// can't actually do prog change till start of next block, as some upstream ug's may have already processed 'this' block
		timestamp_t next_block_start_clock =  UGClones.front()->AudioMaster()->NextGlobalStartClock();

		for (auto u : UGClones)
		{
			// because ugs get moved around during build process(inline expansion), they may end up in different container
			if( u->patch_control_container == p_patch_control_container )
			{
				u->QueProgramChange( next_block_start_clock, patch );
			}
		}
	}
}


ug_base* Voice::findIoMod()
{
	for( auto& u : UGClones)
	{
		if(u->GetFlags() & UGF_IO_MOD)
		{
			return u;
		}
	}

	return nullptr;
}

void Voice::SetUnusedPlugs2()
{
#ifdef VOICE_USE_VECTOR
	for (int i = 0; i < UGClones.size(); ++i) // iterate by index because inserting a module invalidates iterators.
	{
		auto u = UGClones[i];
#else
	for (auto u : UGClones)
	{
#endif
		u->SetUnusedPlugs2();
	}
}

// This container may contain child containers that have been 'expanded inline'
// if so, they will have no children of their own.  The child containers and their
// io ugs can be deleted.  But first their connections should be re-routed
void Voice::ReRouteContainers()
{
	for( auto ug : UGClones )
	{
		ug->ReRoutePlugs();
	}
}

// This container may contain child containers that have been 'expanded inline'
// if so, they will have no children of their own.  The child containers and their
// io ugs can be deleted.  But first there connections should be re-routed
void VoiceList::ReRouteContainers()
{
	for (auto v : *this)
	{
		v->ReRouteContainers();
	}
}

void Voice::ExportUgs(std::list<class ug_base*>& ugs)
{
	for (auto u : UGClones)
	{
		ug_container* c = dynamic_cast<ug_container*>(u);

		if(c)
		{
			c->ExportUgs(ugs);
		}

		ugs.push_back(u);
	}
}

void VoiceList::ExportUgs(std::list<class ug_base*>& ugs)
{
	for (auto v : *this)
	{
		v->ExportUgs(ugs);
	}
}

// sort into correct order, so they are opened in order.
// Biggest order to smallest (most downstream)
// Note SortOrder2 is smallest upstream (opposit of old sort order)
void Voice::Sort3()
{
	UGClones.sort(
		[](const ug_base* first, const ug_base* second)
	{
		assert(first->SortOrder2 >= 0 && second->SortOrder2 >= 0);
		return first->SortOrder2 < second->SortOrder2;
	}
	);
}

void Voice::CheckSortOrder()
{
#ifdef _DEBUG
	int last = -10;

	for( auto it = UGClones.rbegin() ; it != UGClones.rend() ; ++it )
	{
		ug_base* u = *it;
		assert( u->SortOrder2 < last );
		last = u->SortOrder2;
	}

#endif
}

void VoiceList::suspendVoice(timestamp_t timestamp, int physicalVoice)
{
	if (physicalVoice > 0)
	{
		auto voice = at(physicalVoice);

		// Check new note hasn't soft-stolen voice since it went quiet.
		if (!voice->isHeld())
		{
			voice->Suspend2(timestamp);
			OnVoiceSuspended(timestamp);
		}
	}
}

// Fast-mute a playing voice. Used by Hihat Exclusive modules.
void VoiceList::killVoice( timestamp_t timestamp, /*int channel,*/ int voiceId )
{
	// Search Voice list for unused voice
	for (auto it = begin() + 1; it != end(); ++it)
	{
		auto v = *it;
		// allocate any suspended voice.
		if( !v->IsSuspended() )
		{
			// find voice to steal from active voices (not already stolen).
			if( v->voiceState_ == VS_ACTIVE && v->NoteNum == voiceId )
			{
				// kill old voice playing same note.
				// Set voice state to muting.
				v->NoteMute(timestamp);

				const float voiceActive = 0.0f;
				SetVoiceParameters( timestamp, v, voiceActive );

				// de-allocate voice.
				VoiceAllocationNoteOff(timestamp, /*channel,*/ v->NoteNum);
				v->deactivate(timestamp);
			}
		}
	}
}

bool isAvailable(Voice* v)  
{
	return v->IsSuspended() || v->IsRefreshing();
}

Voice* VoiceList::allocateVoice( timestamp_t timestamp, /*int channel,*/ int voiceId, int voiceAllocationMode, bool steal)
{
	/*
	When benchwarmer (reserve voices) exausted - when Voice Stealing occurs the new note is "Held back"
	to allow a very short Fade-Out of the old Voice (<10ms) before the new note steals the voice
	*/
#if defined( DEBUG_VOICE_ALLOCATION )
	std::ostringstream debugAllocateReason;
	std::ostringstream debugAllocateAdditional;
	debugAllocateReason << "\nALLOCATE Note:" << voiceId << " :";

	std::string allocatedCaret_before;
	allocatedCaret_before.assign(size(), ' ');
	for (int i = 1; i < size(); ++i)
	{
		auto v = at(i);
		switch (v->voiceState_)
		{
		case VS_SUSPENDED:
			allocatedCaret_before[v->m_voice_number] = '.';
			break;
		case VS_ACTIVE:
			if(v->NoteNum == -1)
				allocatedCaret_before[v->m_voice_number] = 'r'; // Refresh
			else
				allocatedCaret_before[v->m_voice_number] = 'a';
			break;
		case VS_MUTING:
			allocatedCaret_before[v->m_voice_number] = 'm';
			break;
		};
	}
#endif

	Voice* allocatedVoice{};
	Voice* stealVoice{};

	if(voice_allocation::isMonoMode(voiceAllocationMode))
	{
		constexpr int MONO_VOICE_NUMBER = 1;
		allocatedVoice = at(MONO_VOICE_NUMBER);
		
		#if defined( DEBUG_VOICE_ALLOCATION )
			debugAllocateReason << "V1 (mono mode)";
		#endif
	}
	else // allocate best voice
	{
		// cycle through voices (allows sample-playing voices to unload samples used by previous patch).
		if (++nextCyclicVoice_ == size())
			nextCyclicVoice_ = 1; // voice 0 reserved for mono modules.

		auto ncv = at(nextCyclicVoice_);
		
		// RULE 1: Allocate next cyclic voice (if available)
		if ( isAvailable(ncv) )
		{
			allocatedVoice = ncv;

			#if defined( DEBUG_VOICE_ALLOCATION )
				debugAllocateReason << " [cyclic " << nextCyclicVoice_ << "]";
			#endif
		}
		else
		{
			#if defined( DEBUG_VOICE_ALLOCATION )
				debugAllocateReason << " [cyclic in use " << (int) ncv->voiceState_ << ", " << ncv->NoteNum << "]";
			#endif

			// RULE 2: Allocate any available voice
			for (auto it = begin() + 1; it != end(); ++it)
			{
				auto v = *it;
				if (isAvailable(v))
				{
					allocatedVoice = v;
					break;
				}
			}
		}

		// handle voices playing same note number
		for (auto it = begin() + 1; it != end(); ++it)
		{
			auto v = *it;

#if defined( DEBUG_VOICE_ALLOCATION )
			if (v->NoteNum == voiceId && v->voiceState_ != VS_ACTIVE)
			{
				debugAllocateReason << " [softsteal fail. State=" << (int) v->voiceState_ << "]";
			}
#endif

			if (v->NoteNum == voiceId && v->voiceState_ == VS_ACTIVE)
			{
				const auto mode = voiceAllocationMode & 0x07;
				
				if (mode == VA_POLY_SOFT)
				{
					// RULE 3: In poly-soft mode, allocate any playing voice with same note number
					allocatedVoice = v;
#if defined( DEBUG_VOICE_ALLOCATION )
					debugAllocateReason << " [softsteal same note]";
#endif
				}
				else if (mode == VA_POLY_OVERLAP)
				{
					// In standard overlap mode, old voice is forced to release, but with a gradual rate
					
					if (v->isHeld()) // Not likely two same keys held unless receiving on two channels.
					{
						v->NoteOff(timestamp);
					}

					// Force gate off in case voice is held with sustain pedal.
					SetVoiceParameters(timestamp, v, 0.5f);
				}
				else if (mode == VA_POLY_HARD)
				{
					// in hard steal mode, ensure previous voice is shut down (fast).
					stealVoice = v;
				}
			}
		}
		
		// count active voices, and overlapped voices.
		int activeVoiceCount = 0;
		unsigned char overlappedKeys[maxVoiceId]{};
		for (auto it = begin() + 1; it != end(); ++it)
		{
			auto v = *it;
			if (v == allocatedVoice) // the voice we just allocated (may not have it's note-number yet.
			{
				++activeVoiceCount;

				assert(voiceId >= 0 && voiceId < std::size(overlappedKeys));
				overlappedKeys[voiceId]++;
			}
			else if(v->voiceState_ == VS_ACTIVE && !v->IsRefreshing()) // a playing voice.
			{
				++activeVoiceCount;

				assert(v->NoteNum >= 0 && v->NoteNum < std::size(overlappedKeys));
				overlappedKeys[v->NoteNum]++;
			}
		}
		
		const float recipricolSampleRate = 1.0f / ((ug_container*)this)->AudioMaster()->SampleRate();

		// Guy-overlap mode, allow up to 3 voices to play same note number naturally, older ones are more aggressivly cut off.
		// Restrict number of overlaps on a given key.
		if ((voiceAllocationMode & 0x07) == VA_POLY_OVERLAP_GUYR)
		{
			const int maxOverlapVoicesPerKey = 3;

			if (overlappedKeys[voiceId] >= maxOverlapVoicesPerKey)
			{
				Voice* quietestVoice{};
				const float attackSamples = 0.2f * recipricolSampleRate;
				timestamp_t notesStartedBefore = timestamp - (timestamp_t)attackSamples;
				float quietest = 100.0f;
				for (auto it = begin() + 1; it != end(); ++it)
				{
					auto v = *it;
					if (v->NoteNum == voiceId && v->voiceState_ == VS_ACTIVE && v->voiceActive_ == 1.0f)
					{
						if (v->NoteOnTime < notesStartedBefore && v->peakOutputLevel < quietest)
						{
							quietest = v->peakOutputLevel;
							quietestVoice = v;
						}
					}
				}

				if (quietestVoice)
				{
					SetVoiceParameters(timestamp, quietestVoice, 0.5f); // VoiceActive 0.5 means it's a secondary overlap of another voice, don't steal but fade out despite sustain pedal.
				}
			}
		}
		
#if defined( DEBUG_VOICE_ALLOCATION )
		if (allocatedVoice)
		{
			debugAllocateReason << "V" << allocatedVoice->m_voice_number;

			if (ncv->IsRefreshing())
				debugAllocateReason << " (Refreshing)";
			else
			{
				if (allocatedVoice->voiceState_ != VS_ACTIVE)
					debugAllocateReason << " (Suspended)";
			}
			
			if (allocatedVoice == ncv)
				debugAllocateReason << ". Cyclic priority";
		}
#endif
		if (!stealVoice)
		{
			// when polyphony limit is exceeded or we failed to allocate, steal a voice.
			if (activeVoiceCount > Polyphony || !allocatedVoice)
			{
				Voice* best_off{};  // best unused voice to steal (or nullptr if none)
				Voice* best_on{};	// best 'in-use' voice to steal
				// note better to use incrementing value (eash note-on increments value, saves relying on 32 bit timestamp).
				// compare sounding note's timestamp.  Any started before 'now' can be stolen.
				// notes started on SAME timestamp can't be stolen because voiceactive messages will ovewright each other causing ADSR to NOT reset.(clicks)
				float bestHeldNoteScore = -100.0f;
				float bestReleasedNoteScore = -100.0f;

				// Search Voice list for unused voice
				for (auto it = begin() + 1; it != end(); ++it)
				{
					auto v = *it;

					// determine best voice to steal from active voices (not already stolen).
					if (v == allocatedVoice || v->voiceState_ != VS_ACTIVE || v->IsRefreshing())
						continue;

					// steal the oldest/quetest voice.
					const float loudnessEstimate = (std::min)(1.0f, v->peakOutputLevel);
					const float scoreQuietness = 1.0f - loudnessEstimate;

					// Assume attack is clicky or perceptually significant to percieved loudness. Also compensates for 10ms lag updating peak level.
					// attack will be more important up to 0.2 seconds into note, then loudness takes precedence.
					const float noteDuration = (timestamp - v->NoteOnTime) * recipricolSampleRate;
					const float noteDuration_score = std::min(0.5f, 0.1f * noteDuration);

					// note attack estimated at 0.2 seconds (1 / 5.0).
					const float attack_boost = 0.5f * (std::max)(0.0f, 1.0f - noteDuration * 5.0f);

					// Score increases 10% for each overlap.
					const float overlap_score = (std::min)(1.0f, (overlappedKeys[v->NoteNum] - 1) * 0.1f);

					const float totalScore = scoreQuietness - attack_boost + noteDuration_score + overlap_score;

					_RPTN(0, "Voice %d, noteDuration %f Score %f\n", v->m_voice_number, noteDuration, totalScore);

					if (v->NoteOffTime == SE_TIMESTAMP_MAX) // Held Note
					{
						if (v->NoteOnTime < timestamp) // can't steal a voice that only *just* started.
						{
							if (bestHeldNoteScore < totalScore)
							{
								best_on = v;
								bestHeldNoteScore = totalScore;
							}
						}
					}
					else // released note.
					{
						if (bestReleasedNoteScore < totalScore)
						{
							best_off = v;
							bestReleasedNoteScore = totalScore;
						}
					}
				}

				// shutdown a voice if too many active.
				// Not if voice allocation failed because note will be help-back till benchwarmer free. Unless no benchwarmers at all.
				// Determine best note to 'steal'
				if (best_off)
				{
					stealVoice = best_off;
				}
				else
				{
					stealVoice = best_on;
				}

				// Sometimes there's no voice available to steal because all the active voices started on
				// exact same timestamp (can't steal those because voice-active signal timestamps would co-incide/cancel).
				// In this case, we can't allocate any voice (even if benchwarmer free) because that would exceed polyphony.
				if (!stealVoice) // is available.
				{
					// no steal voice available.
					allocatedVoice = nullptr;

#if defined( DEBUG_VOICE_ALLOCATION )
					loggingFile << "No Voice allocated - exceeded polyphony already on this timestamp.\n";
					_RPT0(_CRT_WARN, "No Voice allocated - exceeded polyphony already on this timestamp.\n");
#endif
				}
			}
		}

		if (stealVoice) // is available.
		{
			// In Guy's mode is like overlap, except stolen voices are not held-back, just re-used as-is. (soft takeover).
			if (allocatedVoice == 0 && (voiceAllocationMode & 0x07) == VA_POLY_OVERLAP_GUYR)
			{
				allocatedVoice = stealVoice;
				stealVoice = nullptr;
			}
			else
			{
				if (steal)
				{
					// if we failed to allocate (no reserve voices), cut off stolen voice ASAP (-1.0), else can do so gently (0.0).
					const float voiceActive = allocatedVoice ? 0.0f : -1.0f;

#if defined( DEBUG_VOICE_ALLOCATION )
					debugAllocateAdditional << "    Stole V" << stealVoice->m_voice_number;

					if (voiceActive < 0.0f)
					{
						debugAllocateAdditional << " (fast)\n";
					}
					else
					{
						debugAllocateAdditional << " (slow)\n";
					}
#endif

					// Special case for two note-ons on same timestamp.
					// Can't immediately mute voice because Voice-Muter hasn't had chance to get Voice/Active signal.
					// increase timestamp a little.
					timestamp_t ts = timestamp;

					if (stealVoice->NoteOnTime == ts)
					{
						++ts;
					}

					// Set voice state to muting.
					stealVoice->NoteMute(ts);

					// de-allocate voice.
					// replaced VoiceAllocationNoteOff() with DoNoteOff() to avoid affecting heldback notes in overlap mode. We need to be able to steal a voice without affecting a heldback note on the same key.
					DoNoteOff(ts, stealVoice, voiceActive);
				}
				else
				{
					stealVoice = {};
				}
			}
		}
	}
	
	if( allocatedVoice )
	{
		allocatedVoice->peakOutputLevel = 1.0f; // until actual value arrives.
		allocatedVoice->outputSilentSince = std::numeric_limits<timestamp_t>::max();
	}
	
#if defined( DEBUG_VOICE_ALLOCATION )

	if( allocatedVoice == 0 )
	{
		debugAllocateReason << "NONE";
	}

	_RPT1(_CRT_WARN, "%s\n", debugAllocateReason.str().c_str() );
	loggingFile << debugAllocateReason.str() << "\n";

	if( !debugAllocateAdditional.str().empty() )
	{
		_RPT1(_CRT_WARN, "%s", debugAllocateAdditional.str().c_str() );
		loggingFile << debugAllocateAdditional.str() << "\n";
	}

	std::string allocatedCaret;
	allocatedCaret.assign(size(), ' ');
	for (int i = 1; i < size(); ++i)
	{
		auto v = at(i);
		switch (v->voiceState_)
		{
		case VS_SUSPENDED:
			allocatedCaret[v->m_voice_number] = '.';
			break;
		case VS_ACTIVE:
			if (v->NoteNum == -1)
				allocatedCaret[v->m_voice_number] = 'r'; // Refresh
			else
				allocatedCaret[v->m_voice_number] = 'a';
			break;
		case VS_MUTING:
			allocatedCaret[v->m_voice_number] = 'm';
			break;
		};
	}

	if( allocatedVoice )
	{
		allocatedCaret[allocatedVoice->m_voice_number] = 'A';
	}

	if (stealVoice)
	{
		allocatedCaret[stealVoice->m_voice_number] = 'X';
	}
	
	_RPT1(_CRT_WARN, "%s   (before)\n", allocatedCaret_before.c_str());
	_RPT1(_CRT_WARN, "%s   (after)\n", allocatedCaret.c_str());

	loggingFile << allocatedCaret_before << "   (before)\n";
	loggingFile << allocatedCaret << "   (after)\n";

#endif

	return allocatedVoice;
}

void VoiceList::DoNoteOn(timestamp_t timestamp, Voice* voice, int voiceId, bool sendTrigger, bool autoGlide)
{
	// mono-modes, and soft poly re-uses a playing voice of the same note-number without resetting it.
	// hard poly resets every voice on every note.
	// bool hardReset = ( (voiceAllocationMode & 0xff) == 1 /*PolyHard*/ ) || voice->IsSuspended();
	// no also needs setting if muting... bool hardReset = voice->voiceState_ == VS_SUSPENDED;
	const bool hardReset = voice->voiceState_ != VS_ACTIVE || voice->IsRefreshing();
	
	voice->NoteNum = static_cast<short>(voiceId);
	voice->activate(timestamp, /*channel,*/ voiceId);

	// bring poly controller values up-to-date for new voice.
	auto thisContainer = static_cast<ug_container*>( this );
	float voicePitch = thisContainer->get_patch_manager()->InitializeVoiceParameters(thisContainer, timestamp, voice, sendTrigger);

	float voiceActive = voice->voiceActive_; // default is to leave it alone.
	if( hardReset )
	{
		voiceActive = 1.0f;
	}
	SetVoiceParameters(timestamp, voice, voiceActive, voiceId);

	// Glide.
	{
		// Get glide type.
		int lVoiceAllocationMode;
		if( overridingVoiceAllocationMode_ != -1 )
		{
			lVoiceAllocationMode = overridingVoiceAllocationMode_;
		}
		else
		{
			lVoiceAllocationMode = voiceAllocationMode_;
		}

		autoGlide |= ( ( lVoiceAllocationMode >> 16 ) & 0x01 ) == 1; // Always glide mode.

		// If auto-glide not triggered, just jump to pitch.
		if( autoGlide && mrnPitch >= -999 && portamento_ > 0.0f)
		{
			// Calculate new glide time in samples.
			const float time = thisContainer->getSampleRate() * powf(10.0f, (portamento_ * 0.4f) - 3.0f);
			//_RPT1(_CRT_WARN, "\nVoiceList:Glidetime %fs\n", powf(10.0f, (portamento_ * 0.4f) - 3.0f));

			const float minimumNonZeroGlide = 1.0f; // Prevent floating divide-by-zero exception due to glide time being VERY small.
			if (time > minimumNonZeroGlide)
			{
				// Calculate where glide has reached since last note-on.
				auto delta = timestamp - mrnTimeStamp;

				//_RPT3(_CRT_WARN, "VoiceList:was Gliding %f -> %f for %fs \n", mrnPitch, portamentoTarget, (float) (delta / thisContainer->SampleRate()));

				mrnPitch += portamentoIncrement * (float)delta;

				// If we've had enough time to reach target, clamp to target.
				if (portamentoIncrement > 0.0f)
				{
					if (mrnPitch > portamentoTarget)
					{
						mrnPitch = portamentoTarget;
					}
				}
				else
				{
					if (mrnPitch < portamentoTarget)
					{
						mrnPitch = portamentoTarget;
					}
				}

				// Calculate the NEW increment.
				//_RPT1(_CRT_WARN, "VoiceList:Glide Pitch now %f\n", mrnPitch);
				//auto constantRateGlide = ( lVoiceAllocationMode >> 18 ) & 0x01;lVoiceAllocationMode
				const bool constantRateGlide = GT_CONST_RATE == voice_allocation::extractBits(lVoiceAllocationMode, voice_allocation::bits::GlideRate_startbit, 1);
				if (constantRateGlide)
				{
					// Distance fixed at 1 octave.
					if (voicePitch > mrnPitch)
					{
						portamentoIncrement = 1.0f / time;
					}
					else
					{
						portamentoIncrement = -1.0f / time;
					}
				}
				else
				{
					portamentoIncrement = (voicePitch - mrnPitch) / time;  //difference v
				}
				//_RPT2(_CRT_WARN, "VoiceList:New target %f, inc %f\n", voicePitch, portamentoIncrement);
			}
			else
			{
				portamentoIncrement = 0.0f;
				mrnPitch = voicePitch;
			}
		}
		else
		{
			portamentoIncrement = 0.0f;
			mrnPitch = voicePitch;
		}

		// Send glide start-pitch automation
		const int automation_id = ( ControllerType::GlideStartPitch << 24 ) | voiceId;
		const float normalised = mrnPitch * 0.1f;
		thisContainer->get_patch_manager()->vst_Automation(thisContainer, timestamp, automation_id, normalised);

		portamentoTarget = voicePitch;
		mrnTimeStamp = timestamp;
	}
}

// Sets voice note-off time.
// sends a UET_NOTE_OFF to NoteSource.
void VoiceList::DoNoteOff( timestamp_t timestamp, Voice* voice, float voiceActive )
{
	SetVoiceParameters( timestamp, voice, voiceActive );
    
	voice->NoteOff( timestamp );
}

void VoiceList::VoiceAllocationNoteOff( timestamp_t timestamp, /*int channel,*/ int voiceId)//, int voiceAllocationMode )
{
	int lVoiceAllocationMode;
	if (overridingVoiceAllocationMode_ != -1)
	{
		lVoiceAllocationMode = overridingVoiceAllocationMode_;
	}
	else
	{
		lVoiceAllocationMode = voiceAllocationMode_; //  thisContainer->get_patch_manager()->getVoiceAllocationMode();
	}

	// cancel any held-back notes on this note-number.
	noteStack.erase(voiceId);

	// handle mono-note priority
	if( note_status[voiceId] )
	{
		note_status[voiceId] = false;
		--keysHeldCount;
	}

	if( voiceId == monoNotePlaying_ )
		monoNotePlaying_ = -1;

	bool monoMode = voice_allocation::isMonoMode(lVoiceAllocationMode); // (lVoiceAllocationMode & 0xff) >= VA_MONO;
	if( monoMode )
	{
		int notePriority = ( lVoiceAllocationMode >> 8 ) & 0x03;
		// update note memory (needed for re-trigger handling).
		int replacementVoiceId = -1;

		// releasing last (playing) note?
		if( note_memory[note_memory_idx] == voiceId )
		{
			char last_note;

			do
			{
				note_memory_idx--;

				if( note_memory_idx < 0 )
					note_memory_idx = MCV_NOTE_MEM_SIZE - 1;

				last_note = note_memory[note_memory_idx];
				note_memory[note_memory_idx] = -1;

				if( last_note >= 0 && note_status[last_note] )
				{
					note_memory_idx--;

					if( notePriority == NP_LAST )
						replacementVoiceId = last_note;

					break;
				}
			}
			while(last_note >= 0);
		}

		if( notePriority != NP_NONE )
		{
			switch(notePriority)
			{
			case NP_LOWEST:
				for (int i = 0; i < maxVoiceId; i++) // find lowest note
				{
					if( note_status[i] )
					{
						if( i > voiceId )
							replacementVoiceId = i; // only jump if note-off was current lowest

						break;
					}
				}

				break;

			case NP_HIGHEST:
				for (int i = maxVoiceId-1; i >= 0; i--)
				{
					if( note_status[i] )
					{
						if( i < voiceId )
							replacementVoiceId = i;

						break;
					}
				}

				break;
			};

			if( replacementVoiceId >= 0 )
			{
				note_memory_idx++;
				note_memory_idx %= MCV_NOTE_MEM_SIZE;
				note_memory[note_memory_idx] = static_cast<char>(replacementVoiceId);

				const bool sendTrigger = !voice_allocation::isMonoMode(lVoiceAllocationMode) || voice_allocation::isMonoRetrigger(lVoiceAllocationMode);
				monoNotePlaying_ = replacementVoiceId;
				Voice* voice = allocateVoice(timestamp, /*channel,*/ replacementVoiceId, lVoiceAllocationMode, true);
				voice->voiceActive_ = 1.0f; // not sure.

				const bool autoGlide = true; // because we had 2 or more keys held
				DoNoteOn(timestamp, voice, replacementVoiceId, sendTrigger, autoGlide);
				return;
			}
		}
	}
	
	NoteOff(timestamp, static_cast<short>(voiceId));
}

void VoiceList::OnVoiceSuspended( timestamp_t /*p_clock*/ )
{
	if( !noteStack.empty() )
	{
		// Not safe to restart voice right here because currently executing ug is likely to a midi-cv or keyboard2, and be
		// re-activated (causing it to be re-sorted into running ug list, causing some
		// running ugs with same sort-order to be skipped and crash.
		// Schedule PlayHeldBackNotes() on next block start of DspPatchManager (NOT proxy, which will crash).

		// get 'real' patch manager
		auto container = static_cast<ug_container*>(this);
		auto am = container->AudioMaster();
		while (dynamic_cast<DspPatchManager*>(container->get_patch_manager()) == nullptr)
		{
			container = dynamic_cast<ug_oversampler*>(am)->ParentContainer();
		}

		ug_base* u = container->GetParameterSetter(); 

		// spit pointer into two 32-bit values.
		union notNice
		{
			ug_container* c;
			int32_t raw[2];
		} pointerToContainer;

		pointerToContainer.c = container;

#if defined( SE_FIXED_POOL_MEMORY_ALLOCATION )
		u->AddEvent( u->AudioMaster()->AllocateMessage(
#else
		u->AddEvent(new_SynthEditEvent
#endif
			(
				container->AudioMaster()->NextGlobalStartClock(), // IMPORTANT to use outer containers time
				UET_PLAY_WAITING_NOTES,
				pointerToContainer.raw[0],
				pointerToContainer.raw[1]
			)
		);
		return;
	}
}


void HostVoiceControl::ConnectPin( UPlug* pin )
{
	pin->SetFlag(PF_POLYPHONIC_SOURCE); // so any latency compensators get cloned.
    outputPins_.push_back(pin);
}

void HostVoiceControl::sendValue( timestamp_t clock, ug_container* container, int physicalVoiceNumber, int32_t size, void* data )
{
    for( auto plug : outputPins_ )
    {
        timestamp_t timestamp = plug->UG->ParentContainer()->CalculateOversampledTimestamp( container, clock );

		if (plug->GetFlag(PF_PARAMETER_1_BLOCK_LATENCY))
		{
			timestamp += plug->UG->AudioMaster()->BlockSize();
		}

        plug->TransmitPolyphonic( timestamp, physicalVoiceNumber, size, data );
	}
}
