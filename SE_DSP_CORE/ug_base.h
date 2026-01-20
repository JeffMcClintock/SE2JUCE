#pragma once

#include <list>
#include <functional>
#include "UPlug.h"
#include "ug_flags.h"
#include "EventProcessor.h"
#include "sample.h"
#include "ISeAudioMaster.h"

class ULookup;
class ULookup_Integer;
class SeAudioMaster;
class Module_Info;
class CUGLookupList;
class ug_container;
class IDspPatchManager;
class my_input_stream;

// Handy macros for enum type data
#define MIDI_CHAN_LIST1 L"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16"
#define MIDI_CHAN_LIST L"All=-1," MIDI_CHAN_LIST1

// return info about plugs
#define	DECLARE_UG_INFO_FUNC2	void ListInterface2(InterfaceObjectArray &PList) override
#define	IMPLEMENT_UG_INFO_FUNC2(class_name) void class_name::ListInterface2(InterfaceObjectArray &PList)

// static and dynamic contructors
#define	DECLARE_UG_BUILD_FUNC(class_name) static ug_base * CreateObject(){return new class_name;}; ug_base * Create() override {return CreateObject();}

// Macros to be used to init a Interface Object in unit_gen::ListInterface
// more eficient in terms of code size.
#define LIST_PIN( P1,P3,P5,P6,P7,P8 ) ListPin(PList, nullptr, P1, P3, DT_FSAMPLE,(P5),(P6),P7,(P8) );
#define LIST_PIN2( P1,P1B,P3,P5,P6,P7,P8 ) ListPin(PList, nullptr, P1, P3, DT_FSAMPLE, (P5), (P6), P7, (P8), &(P1B) );
#define LIST_VAR3( P1,P2,P3,P4,P5,P6,P7,P8 ) ListPin(PList, (void *) &P2, (P1),P3,P4,(P5),(P6),P7,(P8) );
// Same, with null variable address.
#define LIST_VAR3N( P1,  P3,P4,P5,P6,P7,P8 ) ListPin(PList, nullptr, (P1),P3,P4,(P5),(P6),P7,(P8) );
// GUI-only plug
#define LIST_VAR_UI( P1,   P3,P4,P5,P6,P7,P8 ) ListPin(PList, nullptr, (P1),P3,P4,(P5),(P6),P7,(P8) );

#define MIDDLE_A  69
const float PI = 3.14159265358979323846f;
const float PI2 = (2.0f * PI);

#define VoltageToFrequency(freq) ( 440.f * powf(2.f, (freq) - 5.f ) )
// !!that division could be replaced with multiplication
// old #define FrequencyToVoltage(freq) ( logf(freq) / 0.69314718055994529f - 3.7813597135246599f)
// precise definition is: LOG(B13/440)/LOG(2) + 5 Simplified is: LOG(freq*4/55)/LOG(2)
#define FrequencyToVoltage(freq) ( 1.442695041f * logf( 0.07272727273f * freq ) )

// define msgs sent from UI to ug via NotifyUg()
//#define NUG_ON_DEBUG_INFO			0
//#define NUG_UPDATE_OUTPUT			1
#define NUG_INITIAL_UPDATE_OUTPUT	2
#define NUG_NOTE_ON					3
#define NUG_NOTE_OFF				4
//#define NUG_DEBUG_RUN_LIST			6
#define NUG_UPDATE_ENV				7
#define NUG_VST_AUTO_ID_CHANGE		8
#define NUG_RELOAD_FILE				9

// static cast much safer than old style.. (process_func_ptr)
#define DEBUG_TIMESTAMP
#define SET_PROCESS_FUNC(func)	process_function = static_cast <process_func_ptr> (func);

// backward compat
#define SET_CUR_FUNC(func) SET_PROCESS_FUNC(func)

class ug_base;

typedef void (ug_base::* process_func_ptr)(int start_pos, int sampleframes); // Pointer to sound processing member function

enum SetDownstreamErrors { SDS_OK, SDS_POLYPHONIC_FEEDBACK };

// List of modules involved in feedback path.
struct feedbackPin
{
	feedbackPin(UPlug* pin);
	int32_t moduleHandle;
	int32_t pinIndex;
	std::wstring debugModuleName;
};

struct FeedbackTrace
{
	std::list< std::pair<feedbackPin, feedbackPin> > feedbackConnectors;
	int reason_;

	FeedbackTrace(int reason) : reason_(reason) {}
	void AddLine(UPlug* from, UPlug* to);

	void DebugDump();
};

void CreateMidiRedirector(ug_base* midiCv);

class ug_base : public EventProcessor
{
	friend class Module_Info;
public:
	virtual void DoProcess(int buffer_offset, int sampleframes) override;
	void process_nothing(int start_pos, int sampleframes);
	void process_sleep(int start_pos, int sampleframes);

	template <typename PointerToMember>
	void setSubProcess(PointerToMember functionPointer)
	{
		process_function = static_cast <process_func_ptr> (functionPointer);
	}

	void SetUnusedPlugs2();
	// construction
	ug_base();
	virtual ~ug_base();
	virtual ug_base* Create() = 0;
	virtual ug_base* Clone( CUGLookupList& UGLookupList );
	virtual ug_base* Copy( ISeAudioMaster* audiomaster, CUGLookupList& UGLookupList );
	virtual void BuildHelperModule() {}

	virtual void Setup( class ISeAudioMaster* am, class TiXmlElement* xml );
	void SetupWithoutCug();
	void DeleteAllPlugs();
	virtual void QueProgramChange( timestamp_t p_clock, int p_patch_num ); // SDK2 only

	void ResetStaticOutput();
	void SleepMode();
	void SleepIfOutputStatic(int sampleframes)
	{
		static_output_count -= sampleframes;

		if( static_output_count <= 0 )
		{
			SET_CUR_FUNC( &ug_base::process_sleep );
		}
	}
	bool IsSuspended()
	{
		return (flags & UGF_SUSPENDED) != 0;
	}

	inline void Suspend()
	{
		// this should only be called from ug_base_ex in response to UET_SUSPEND events
		// should not be called directly by module.

		if(flags & UGF_NEVER_SUSPEND)
			return;

		AudioMaster()->SuspendModule(this);
		SetFlag(UGF_SUSPENDED);
	}

	virtual void Resume();

	// debug
#if defined( _DEBUG )
	void DebugIdentify(bool withHandle = false);
	void DebugPrintName(bool withHandle = false) override;
	void DebugPrintContainer(bool withHandle = false) override;
	void DebugVerfifyEventOrdering();
#else
	inline void DebugIdentify(bool /*withHandle*/ = false) {}
	inline void DebugPrintName(bool /*withHandle*/ = false) {}
	inline void DebugPrintContainer(bool /*withHandle*/ = false) {}
#endif
	void VerifyOutputsConsistant();
	void SumCpu(float cpu_block_rate);
	virtual void OnCpuMeasure(float /*cpu_block_rate*/) {} // containers and oversamplers only
	void AttachDebugger();
	std::wstring DebugModuleName( bool andHandle = false );
	std::wstring GetFullPath();

	// new
	void SetPinValue(timestamp_t timestamp, int pin_index, int datatype, const void* data1, int data_size1);
	void SendPinsCurrentValue( timestamp_t timestamp, UPlug* from_plug );
	// old
	// OutputChange replaced by UPlug::TransmitState()
	void OutputChange(timestamp_t p_sample_clock, UPlug* p_plug, state_type p_new_state )
	{
		p_plug->TransmitState( p_sample_clock, p_new_state );
	}
	virtual void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state);
	virtual bool BypassPin(UPlug* fromPin, UPlug* toPin);
	virtual int calcDelayCompensation();
	SeAudioMaster* AudioMaster2();
	void HandleEvent(SynthEditEvent* e) override;
	void SendPendingOutputChanges();

	//void OnDelConnector(int pinIndex);
	virtual void ListInterface2(InterfaceObjectArray& /*PList*/) {}
	virtual int Open();
	virtual int Close();

	// communication
	void message(std::wstring msg, int typ = 0);
	virtual void OnUiNotify2(int /*p_msg_id*/, my_input_stream& /*p_stream*/);
	virtual void OnUiMsg(int p_msg_id, my_input_stream& p_stream) override;
	// polyphonic setup
	FeedbackTrace* CalcSortOrder3(int& maxSortOrderGlobal);
	void InsertVoiceAdders();
	bool isPolyLast();
	bool GetPPDownstreamFlag()
	{
		return (flags & UGF_POLYPHONIC_DOWNSTREAM) != 0;
	}
	void SetPolyphonic(bool p);
	void SetPPDownstreamFlag(bool p);
	virtual bool PPGetActiveFlag();
	void FlagUpStream(int flag, bool debugTrace = false);
	virtual struct FeedbackTrace* PPSetDownstream();
	virtual void PPSetUpstream();
	virtual void SetPPVoiceNumber(int n);
	virtual void PPPropogateVoiceNum(int n);
	virtual void PPPropogateVoiceNumUp(int n);
	virtual void CloneContainerVoices() {} // only for containers.
	inline bool GetPolyphonic()
	{
		return (flags & UGF_POLYPHONIC) != 0;
	}
	bool IgnoredByVoiceMonitor()
	{
		return (flags & UGF_VOICE_MON_IGNORE) != 0;  // Module always ignored by voice-monitor. e.g. [Scopes, Freq Analyser2, Volts-to-Float]
	}
	bool GetPolyGen()
	{
		return (flags & UGF_POLYPHONIC_GENERATOR) != 0;
	}	// set if ug spawns new notes (UNotesource)
	bool GetPolyAgregate()
	{
		return (flags & UGF_POLYPHONIC_AGREGATOR) != 0;
	}	// set if ug sums voices (IO Mod )

	// plugs
	void AddPlug( UPlug* plug );
	void AddPlugs(ug_base* copy_ug);
	void AddFixedPlugs(); // not for SDK3 ( see ug_plugin3::AddFixedPlugs() )
	virtual void SetupDynamicPlugs();
	void HookUpParameters();
	void SetupDynamicPlugs2();
	virtual void assignPlugVariable(int /*p_plug_desc_id*/, UPlug* /*p_plug*/) {}
	void connect_oversampler_safe(UPlug * from_plug, UPlug * to_plug);
	void connect( UPlug* from_plug, UPlug* to_plug );
	virtual void CloneConnectorsFrom( ug_base* FromUG, CUGLookupList& UGLookupList );
	/*virtual */ void CopyConnectorsFrom( ug_base* FromUG, CUGLookupList& UGLookupList );
	void SetupClone( ug_base* clone );
	int GetPlugCount()
	{
		return (int) plugs.size();
	}
//	UPlug* GetPlugById(int id);
#if defined( _DEBUG )
	UPlug* GetPlug(int p_index);
#else
	inline UPlug* GetPlug(int p_index)
	{
		return plugs[p_index];
	}
#endif
	UPlug* GetPlug(const std::wstring& plug_name);

	virtual void OnProgChange(short /*chan*/,int /*patch_num*/) {}

	virtual IDspPatchManager* get_patch_manager();
	ug_container* ParentContainer()
	{
		return parent_container;
	}

	void ClearBypassRoutes(int32_t inPlugIdx, int32_t outPlugIdx);
	bool AddBypassRoute(int32_t inPlugIdx, int32_t outPlugIdx);
	bool AddBypassRoutePt2(UPlug* bypassSourcePlug, UPlug* outPlug);

	void CreateSharedLookup2( const std::wstring& id, ULookup * & lookup_ptr, int sample_rate, size_t p_size, bool create = true, SharedLookupScope scope = SLS_ONE_MODULE );
	void CreateSharedLookup2( const std::wstring& id, ULookup_Integer * & lookup_ptr, int sample_rate, size_t p_size, bool create = true, SharedLookupScope scope = SLS_ONE_MODULE );
	int32_t allocateSharedMemory( const wchar_t* table_name, void** table_pointer, float sample_rate, int32_t size_in_bytes, int32_t& ret_need_initialise, int scope = SLS_ONE_MODULE );
	const float* GetInterpolationtable();
	
	int GetFlags()
	{
		return flags;
	}
	virtual bool isEventListEmpty();

	int pp_voice_num;
	std::vector<UPlug*> plugs;

	ug_container* parent_container;
	ug_container* patch_control_container;

	process_func_ptr process_function;

	static ULookup* m_shared_interpolation_table;

	static bool trash_bool_ptr;
	static float* trash_sample_ptr;
	ug_base* m_clone_of; // used for iterating clones
	ug_base* m_next_clone; // used for iterating clones

	// sleep mode variables
	int static_output_count;
	void UpdateInputVariable(SynthEditEvent* e);
	ug_base* CloneOf();

	// Containers collect sum CPU reading of children. However modules are built in-line - often
	// within some higher-level container. This pointer gives original container.
	// Only used in editor, but remains in plugin so we can share the lib
	ug_base* cpuParent;
	static float cpu_conversion_const;
	static float cpu_conversion_const2;

#if defined( _DEBUG )
	std::wstring debug_name;
#endif

	virtual void IterateContainersDepthFirst(std::function<void(ug_container*)>& /*f*/) {}

	Module_Info* getModuleType()
	{
		return moduleType;
	}
	void setModuleType( class Module_Info* p_moduleType )
	{
		moduleType=p_moduleType;
	}
	void setlatencySamples(int platencySamples)
	{
		latencySamples = platencySamples;
	}

	virtual void BypassRedundentModules(int /*voice*/) {}
	virtual void OnBufferReassigned() {}
	virtual void ReRoutePlugs() {}

	int latencySamples;

protected:
	static void ListPin(InterfaceObjectArray& PList, void* addr, const wchar_t* p_name, EDirection p_direction, EPlugDataType p_datatype, const wchar_t* def_val, const wchar_t* unused = L"-1", int flags = 0, const wchar_t* p_comment = L"", float** p_sample_ptr = nullptr);

	class Module_Info* moduleType;
	int cumulativeLatencySamples;
	static const int LATENCY_NOT_SET = 0xffffffff; //  (numeric_limits<int>::min)();
};
