// VoiceList and Voice
// Used by ug_container
// list of the container's UGs, cloned foe each polyphonic voice
#pragma once
#include <map>
#include <vector>
#include <array>
#include <functional>
#include <list>
#include <fstream>
#include "se_types.h"
#include "modules/se_sdk3/hasMidiTuning.h"
#include "HostControls.h"
#include "HostVoiceControl.h"

#ifdef _DEBUG
// See also DEBUG_VOICEWATCHER & VM_DEBUG_OUTPUT
// #define DEBUG_VOICE_ALLOCATION
#endif

class SeAudioMaster;

#define MCV_NOTE_MEM_SIZE 16

class CUGLookupList : public std::map<class ug_base*,class ug_base*>
{
public:
	ug_base* LookupPolyphonic( ug_base* originalModule );
	ug_base* Lookup2( ug_base* originalModule );
};

class ug_container;

enum EVoiceState { VS_SUSPENDED, VS_ACTIVE, VS_MUTING };

// Hard to see any improvement, but seems a bit faster.
// needs fairly thorough testing #define VOICE_USE_VECTOR

class Voice
{
public:
	Voice(ug_container* container, int p_voice_number);
	~Voice();

	void Sort3();
	void CheckSortOrder();
	void Open(class ISeAudioMaster* p_audiomaster, bool setVoiceNumbers);
	void Resume();
	bool IsSuspended()
	{
		assert(suspend_flag == ( voiceState_ == VS_SUSPENDED ));
		return voiceState_ == VS_SUSPENDED;
	}
	bool IsRefreshing() const
	{
		return NoteNum == -1 && voiceState_ == VS_ACTIVE;/*&& voiceActive_ == 1.0f*/;
	}
	inline bool isHeld()
	{
		return NoteOffTime == SE_TIMESTAMP_MAX;
	}
	void CloneFrom(Voice* p_copy_from_voice);
	void MoveFrom(Voice * p_copy_from_voice);
	void CloneContainerVoices();
	void SetUnusedPlugs2();
	void ExportUgs(std::list<class ug_base*>& ugs);
	void ReRouteContainers();
	void SetUpVoiceAdders();
	void SetUpNoteMonitor( bool secondPass = false );
	void RemoveUG( ug_base* ug );
	void DoneCheck(timestamp_t timeStamp);
	ug_base* findIoMod();
	void SendProgChange( ug_container* p_patch_control_container, int patch);
	void NoteOff( timestamp_t p_clock );
	void reassignVoiceId( short noteNumber );
	void NoteMute( timestamp_t p_clock );
	bool SetNoteSource(ug_base* m);
	void Suspend2( timestamp_t p_clock );
	void IterateContainersDepthFirst(std::function<void(ug_container*)>& f);
	void PolyphonicSetup();
	void SetPPVoiceNumber(int n);
	int Close();
	void CloneUGsFrom( Voice* FromVoice, CUGLookupList& UGLookupList );
	void CloneConnectorsFrom( Voice* FromVoice, CUGLookupList& UGLookupList );
	void CopyFrom(ISeAudioMaster* audiomaster, ug_container* parent, Voice* FromVoice, CUGLookupList& UGLookupList);
	ug_base* AddUG( ug_base* ug );

	// New stuff
	void refresh();
	void activate( timestamp_t p_clock, /*int channel,*/ int voiceId );
	void deactivate(timestamp_t timestamp); // entering release phase. Still audible.

#ifdef VOICE_USE_VECTOR
	std::vector<ug_base*> UGClones; // list of ugs for note
#else
	std::list<ug_base*> UGClones; // list of ugs for note
#endif
	timestamp_t NoteOffTime;
	timestamp_t NoteOnTime;
	//short Channel;
	short NoteNum;  // what note is this playing. -1 for refreshing voice
	EVoiceState voiceState_;
	int m_voice_number;
	float peakOutputLevel; // usefull for voice stealing.
	timestamp_t outputSilentSince = std::numeric_limits<timestamp_t>::max();
	float voiceActive_;

private:
#if defined( _DEBUG )
	bool suspend_flag;
#endif
	bool open;
	ISeAudioMaster* m_audiomaster;
	ug_container* container_;
};


class SENoteStack
{
	struct NoteStackEntry
	{
		int midiKeyNumber;
		int usePhysicalVoice;
		timestamp_t timestamp;
		bool canSteal = true; // the first time we attempt to allocate, we can steal. On later attempts don't least we OVER-steal.
	};

	static const int capacity = 128; // power-of-2 please.

	std::vector<NoteStackEntry> notes;
	bool inStack[capacity];

public:
	SENoteStack() // : first(0), last(0)
	{
		memset(inStack, 0, sizeof(inStack));
		notes.reserve(capacity);
	}

	inline bool empty()
	{
		return notes.empty();// last == first;
	}

	void push(timestamp_t timestamp, /*int midiChannel,*/ int key/*, int velocity*/, int physicalVoice)
	{
		if (!inStack[key])
		{
			assert(notes.size() < capacity);

			NoteStackEntry n{ key , physicalVoice , timestamp };
			notes.push_back(n);

			inStack[key] = true;
		}
	}

	inline NoteStackEntry& top()
	{
		return notes.front(); // [first];
	}

	void pop()
	{
		assert(!empty());
		inStack[top().midiKeyNumber] = false;
		notes.erase(notes.begin());
	}

	void erase(int key)
	{
		if (inStack[key])
		{
			inStack[key] = false;

			for (auto it = notes.begin(); it != notes.end() ; ++it)
			{
				if ((*it).midiKeyNumber == key)
				{
					it = notes.erase(it);
					return;
				}
			}
		}
	}
};

class VoiceList : public std::vector<Voice*>, public hasMidiTuning
{
public:
	enum { maxVoiceId = 128 };

	VoiceList( );
	~VoiceList();

	void OpenAll(class ISeAudioMaster* p_audiomaster, bool setVoiceNumbers );
	void SortAll3();
	void CloneVoices();
	void ExportUgs(std::list<class ug_base*>& ugs);

	// new way. Voicelist does polyphonic note management.
	void xNoteOff( timestamp_t p_clock, /*int midiChannel, */int MidiKeyNumber, float velocity, int voiceAllocationMode );
	void xAllNotesOff( timestamp_t p_clock );
	bool HoldPedalState() // only really valid at end of block. For note-monitor only!
	{
		return holdPedalState_;
	}
	void SetHoldPedalState(bool hold) // only really valid at end of block. For note-monitor only!
	{
		holdPedalState_ = hold;
	}
	void DoNoteOn(timestamp_t timestamp, Voice* voice, int voiceId, bool sendTrigger, bool autoGlide);
	void DoNoteOff(timestamp_t timestamp, Voice* voice, float voiceActive = -10.0f);
	void NoteOff(timestamp_t p_clock, /*short chan,*/ short note_num);
	void killVoice(timestamp_t timestamp, /*int channel,*/ int voiceId);
	void suspendVoice(timestamp_t timestamp, int physicalVoice);
	void OnVoiceSuspended( timestamp_t p_clock );
	void ReRouteContainers();
	void SendProgChange( ug_container* p_patch_control_container, int patch );
	Voice* GetVoice( /*short chan,*/ short MidiNoteNumber );
	void AddUG( ug_base* ug );
	void CloseAll();

	// NEW...
	void VoiceAllocationNoteOn(timestamp_t timestamp, /* int midiChannel ,*/ int MidiKeyNumber/*, int velocity*/, int usePhysicalVoice = -1);
	void PlayWaitingNotes(timestamp_t timestamp);
	bool AttemptNoteOn(timestamp_t timestamp, int MidiKeyNumber, int usePhysicalVoice, bool steal);
	void VoiceAllocationNoteOff(timestamp_t timestamp, /*int channel,*/ int voiceId);// , int voiceAllocationMode );
	Voice* allocateVoice( timestamp_t timestamp/*, int channel*/, int voiceId, int voiceAllocationMode, bool steal);

	int voiceReserveCount() const;
	void setVoiceCount(int c);
	void setVoiceReserveCount(int c);
	int getPhysicalVoiceCount();
	void SetVoiceParameters( timestamp_t timestamp, Voice* voice, float voiceActive, int voiceId = -1 );
	void SetContainerPolyphonicCloned()
	{
		is_polyphonic_cloned = true;
	}
	void SetContainerPolyphonicDelayedGate()
	{
		is_polyphonic_delayed_gate = true;
	}	
	bool isContainerPolyphonic() // Distinguish regular containers for ones controlling polyphonic voices.
	{
		// NO. Was being called before notesouce set (when hooking up voice controls).	return front()->NoteSource != 0;
		return is_polyphonic;
	}
	bool isContainerPolyphonicCloned()
	{
		return is_polyphonic_cloned;
	}
	bool isContainerPolyphonicDelayedGate()
	{
		return is_polyphonic_delayed_gate;
	}

	void ConnectVoiceHostControl( HostControls hostConnect, class UPlug* plug );
	bool OnVoiceControlChanged(HostControls hostConnect, int32_t size, const void* data);

	int voiceAllocationMode_;
	int overridingVoiceAllocationMode_;
	float portamento_;

	int getVoiceCount()
	{
		return Polyphony;
	}

	int minimumPhysicalVoicesRequired() const
	{
		return Polyphony + voiceReserveCount();
	}

protected:
	short Polyphony; // used as plug by container, don't change data type from 'short'
	// only used with new-style Keyboard2 voice-allocation.  Must be NULL when using MIDI-CV etc else side-by-side MIDI-CVs won't work.
	int monoNotePlaying_;
	bool note_status[maxVoiceId];
	SENoteStack noteStack;
	char note_memory[MCV_NOTE_MEM_SIZE];
	int note_memory_idx;
	int nextCyclicVoice_;
	int benchWarmerVoiceCount_;

	HostVoiceControl voiceActiveHc_;
	HostVoiceControl voiceIdHc_;

	bool is_polyphonic;
	bool is_polyphonic_cloned;
	bool is_polyphonic_delayed_gate;
	bool holdPedalState_;
	int keysHeldCount;
	float mrnPitch;
	float portamentoIncrement;
	float portamentoTarget;
	timestamp_t mrnTimeStamp;
	int maxNoteHoldBackPeriodSamples;

#ifdef DEBUG_VOICE_ALLOCATION
	std::ofstream loggingFile;
#endif
};
