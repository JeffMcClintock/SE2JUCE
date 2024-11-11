// ug_base.cpp: implementation of the ug_base class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <math.h>
#include "ug_base.h"
#include "ug_container.h"
#include "SeAudioMaster.h"
#include "ULookup.h"
#include "UgDatabase.h"
#include "my_msg_que_output_stream.h"
#include "./modules/se_sdk3/mp_sdk_common.h"
#include "ug_adder2.h"
#include "ug_oversampler.h"
#include "ug_plugin3.h"
#include "UMidiBuffer2.h"
#include "midi_defs.h"
#include "./IDspPatchManager.h"
#include "./dsp_patch_parameter_base.h"
#include "ug_event.h"
#include "ug_patch_param_setter.h"
#include "ug_patch_param_watcher.h"
#include "tinyxml/tinyxml.h"
#include "iseshelldsp.h"
#include "my_input_stream.h"
#include "UniqueSnowflake.h"
#include "./modules/shared/xplatform.h"
#include "HostControls.h"
#include "ug_latency_adjust.h"
#include "ug_latency_adjust_event.h"
#include "./mfc_emulation.h"
#include "UgDebugInfo.h"
#include "SeException.h"

using namespace std;

// #define DEBUG_LATENCY 1

bool ug_base::trash_bool_ptr;
float* ug_base::trash_sample_ptr;

ULookup* ug_base::m_shared_interpolation_table;

float ug_base::cpu_conversion_const;
float ug_base::cpu_conversion_const2;

void ug_base::ListPin(InterfaceObjectArray& PList, void* addr, const wchar_t* p_name, EDirection p_direction, EPlugDataType p_datatype, const wchar_t* def_val, const wchar_t* unused , int flags, const wchar_t* p_comment, float** p_sample_ptr )
{
	PList.push_back(new InterfaceObject(addr, p_name, p_direction, p_datatype, def_val, unused, flags, p_comment, p_sample_ptr));
}

void FeedbackTrace::AddLine(UPlug* from, UPlug* to)
{
	auto os = dynamic_cast<ug_oversampler*>(from->UG);
	if (os)
	{
		from = os->main_container->plugs[from->getPlugIndex()];
	}
	os = dynamic_cast<ug_oversampler*>(to->UG);
	if (os)
	{
		to = os->main_container->plugs[to->getPlugIndex()];
	}
	feedbackConnectors.push_back({ from, to });
}

feedbackPin::feedbackPin(UPlug* pin) :
	moduleHandle(pin->UG->Handle())
	, pinIndex(pin->getPlugIndex())
	, debugModuleName(pin->UG->DebugModuleName())
{}


void FeedbackTrace::DebugDump()
{
#if defined( _DEBUG ) && defined( _WIN32 )
	_RPT0(_CRT_WARN, "\nFEEDBACK PATH\n");

    for( auto it = feedbackConnectors.rbegin(); it != feedbackConnectors.rend(); ++it)
	{
		auto& line = (*it);

		_RPT3(_CRT_WARN, "%S [%d] pin %d -> ", line.first.debugModuleName.c_str(), line.first.moduleHandle, line.first.pinIndex);
		_RPT3(_CRT_WARN, "%S [%d] pin %d\n",  line.second.debugModuleName.c_str(), line.second.moduleHandle, line.second.pinIndex);
	}
	_RPT0(_CRT_WARN, "\n");
#endif
}


#ifdef _DEBUG_MOOSE
static int instanceCounter = 0;
#endif

ug_base::ug_base() : EventProcessor()
,parent_container(0)
,pp_voice_num(-1)
,patch_control_container(NULL)
,m_clone_of(this)
,m_next_clone(0)
,cpuParent(0)
,moduleType(0)
,latencySamples(0)
,cumulativeLatencySamples(LATENCY_NOT_SET)
{
	SET_CUR_FUNC( &ug_base::process_nothing ); // moved here from Open(). as it was overidding derived classes initialisation
	//	SET_CUR_FUNC( &ug_base::process_sleep ); // messed up MIDI somehow
#ifdef _DEBUG_MOOSE
	++instanceCounter;
	_RPT1(_CRT_WARN, "modules instances %d\n", instanceCounter );
#endif
}

ug_base::~ug_base()
{
	DeleteAllPlugs();
#ifdef _DEBUG_MOOSE
	--instanceCounter;
	_RPT1(_CRT_WARN, "modules instances %d\n", instanceCounter );
#endif
}

void ug_base::DeleteAllPlugs()
{
	// Delete all plugs
	for( auto p : plugs )
	{
		delete p;
	}

	plugs.clear();
}

#if defined( _DEBUG )
UPlug* ug_base::GetPlug(int p_index)
{
	assert( plugs[p_index]->getPlugIndex() == p_index );
	return plugs[p_index];
}
#endif

UPlug* ug_base::GetPlugById(int id)
{
	// 99% of the time ID is the same as index.
	if(id >= 0 && id < plugs.size() && plugs[id]->UniqueId() == id)
	{
		return plugs[id];
	}

	for(auto& p : plugs)
	{
		assert( p->UniqueId() != -1 );

		if(p->UniqueId() == id )
			return p;
	}

	// handle awkward SDK3 autoduplicating pins,where ID is relative to first autoduplicating pin.
	// Note that pins store only index and *original* ID (of the first autoduplicating pin)
	if (auto& pindesc = getModuleType()->plugs; !pindesc.empty())
	{
		if (auto& last = pindesc.rbegin()->second; last->autoDuplicate(0))
		{
			const int pinIndex = pindesc.size() + id - last->getPlugDescID(0) - 1;
			if (pinIndex >= 0 && pinIndex < plugs.size())
			{
				return plugs[pinIndex];
			}
		}
	}

	return {};
}

// big problem with dynamic plugs, this code assumes each plug appears
// in same order as ListInterface() produces
UPlug* ug_base::GetPlug(const std::wstring& plug_name)
{
	// old way , phase out getting plug by name
	//InterfaceObjectArray iobs;
	SafeInterfaceObjectArray iobs;
	ListInterface2( iobs );
	int numplugs = (int) iobs.size() - 1;
#ifdef _DEBUG
	bool dynamic_plugs_exist = false;
#endif

	for(int j = 0; j <= numplugs ; j++)
	{
#ifdef _DEBUG

		if( (iobs[j]->GetFlags() & (IO_CUSTOMISABLE|IO_AUTODUPLICATE) ) != 0 )
		{
			dynamic_plugs_exist = true;
		}

#endif

		if( iobs[j]->GetName() == plug_name )
		{
			assert( !dynamic_plugs_exist ); // screws up count
			return GetPlug( j );
		}
	}

	assert(false); // GetPlug(char *name) failed
	return NULL;
}

// see also SetupDynamicPlugs2
void ug_base::SetupDynamicPlugs()
{
	for (auto p : plugs)
	{
		if( p->DataType == DT_FSAMPLE )
		{
			p->CreateBuffer();
		}
	}
}

// As above except creates variables for plugs of all datatypes.
// uses plug's io_variable to hold pointer to data (not seperate list).
// Not used for SDK3.
void ug_base::SetupDynamicPlugs2()
{
	//	_RPT2(_CRT_WARN, "ug_base::SetupDynamicPlugs2 this=%x plugs=%d\n", this, plugs.GetSize() );
	for( auto p : plugs )
	{
		p->CreateBuffer();
	}
}

void ug_base::DoProcess( int buffer_offset, int sampleframes )
{
	assert( sampleframes > 0 );
	assert( !sleeping );
	timestamp_t current_sample_clock = SampleClock();
	timestamp_t end_time = current_sample_clock + sampleframes;
	int start_pos = buffer_offset; // AudioMaster()->Block Pos();
#if defined( _DEBUG )
	DebugVerfifyEventOrdering();
#endif

	for(;;)
	{
		if( events.empty() ) // fast version, when no events on list.
		{
			// don't write over end of block
			assert( start_pos + sampleframes <= AudioMaster()->BlockSize() );

			(this->*(process_function))( start_pos, sampleframes );
			SetSampleClock( end_time );
			return;
		}

		int delta_time = sampleframes;
		SynthEditEvent* next_event = events.front();

		if( next_event->timeStamp < end_time ) // will happen in this block
		{
			assert( current_sample_clock <= next_event->timeStamp );
			delta_time = (int) (next_event->timeStamp - current_sample_clock);
		}

		if( delta_time != 0 )
		{
			(this->*(process_function))( start_pos, delta_time );
			sampleframes -= delta_time;
			current_sample_clock += delta_time;
			SetSampleClock( current_sample_clock );

			if( sampleframes == 0 ) // done
			{
				return;
			}

			start_pos += delta_time;
		}

		// Scan status changes, update variables
		// this must all be done in one go, to keep plugin in consistant state (can't have one variable updating at a time (on same clock))
		for( EventIterator it(events.begin()) ; it != events.end()  ; ++it )
		{
			SynthEditEvent* e2 = *it;
			assert( e2->timeStamp >= current_sample_clock );

			if( e2->timeStamp != current_sample_clock )
			{
				break;
			}

			if( e2->eventType == UET_EVENT_SETPIN )
			{
				UpdateInputVariable(e2);
			}
		}

		// now handle events as per usual
		while( !events.empty() )
		{
			SynthEditEvent* e2 = events.front();
			assert( e2->timeStamp >= current_sample_clock );

			if( e2->timeStamp != current_sample_clock )
			{
				break;
			}

			events.pop_front();
			HandleEvent(e2);

            delete_SynthEditEvent(e2);
		}
	}
}

void ug_base::UpdateInputVariable(SynthEditEvent* e)
{
	UPlug* p = GetPlug(e->parm1);

	// if you put a line into a container into a second container, you get an orphaned plug on the container. need to avoid acessing it's io variable
	if (p->io_variable == 0)
	{
		return;
	}

	int cast_new_value = (int)e->parm3; // new value forced into an int

	switch (p->DataType)
	{
	case DT_FLOAT:
	{
		*((float*)p->io_variable) = (float)*((float*)&cast_new_value);
	}
	break;

	case DT_ENUM:
	{
		*((short*)p->io_variable) = (short)cast_new_value;
	}
	break;

	case DT_INT:
	{
		*((int*)p->io_variable) = cast_new_value;
	}
	break;

	case DT_BOOL:
	{
		*((bool*)p->io_variable) = cast_new_value != 0;
	}
	break;

	case DT_TEXT:
	{
		std::wstring temp((wchar_t*)e->Data(), e->parm2 / sizeof(wchar_t));
		*((std::wstring*)p->io_variable) = (temp);
	}
	break;

	case DT_STRING_UTF8:
	{
		std::string temp((const char*)e->Data(), e->parm2);
		*((std::string*)p->io_variable) = temp;
	}
	break;

	case DT_DOUBLE: // phase out this type. float easier
	{
		*((double*)p->io_variable) = *((double*)e->Data());
	}
	break;

	case DT_MIDI2:
	case DT_FSAMPLE:
		break;

	case DT_BLOB:
	{
		MpBlob* blob = (MpBlob*)p->io_variable;
		blob->setValueRaw(e->parm2, e->Data());
	}
	break;

	default:
		assert(false); // new datatype
		break;
	}
}

int ug_base::Open()
{
	assert( AudioMaster() );
	ResetStaticOutput();
	// add to controllers block list
	AudioMaster()->InsertBlockDev( this );

	// setup sample pointers on behalf of ug
	for( vector<UPlug*>::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
	{
		UPlug* p = *it;

		if( p->sample_ptr )
		{
			*(p->sample_ptr) = p->GetSamplePtr();
		}
	}

	// send myself stat-change events on all inputs
	//  no. how do i know initial values?, InitPlugStates();
	SetFlag( UGF_OPEN );
	AddEvent( new_SynthEditEvent( 0, UET_GRAPH_START , 0, 0,0,0 ) );
	return 0;
}

int ug_base::Close()
{
	SetFlag( UGF_OPEN, false );
	return 0;
}

// Default Clone func, just call object's Build() func
ug_base* ug_base::Clone( CUGLookupList& /*UGLookupList*/ )
{
#ifdef _DEBUG
	if (SeAudioMaster::GetDebugFlag(DBF_TRACE_POLY))
	{
		if (m_next_clone == nullptr) // just print the first one to avoid overkill.
		{
			DebugIdentify(true);
			_RPT0(_CRT_WARN, " Cloned\n");
		}
	}
#endif

	// Deliberately don't call addplugs(), as this is done by calling func
	ug_base* clone = Create();
	SetupClone(clone);
	return clone;
}

ug_base* ug_base::Copy(ISeAudioMaster* audiomaster, CUGLookupList& UGLookupList )
{
	// Deliberately don't call addplugs(), as this is done by calling func
	auto clone = Clone( UGLookupList );
	if( clone )
	{
		// By default they will have same parent pointer and audiomaster as original. fix.
//		clone->parent_container = parent;
		clone->SetAudioMaster(audiomaster);

		UGLookupList.insert( std::pair<ug_base*,ug_base*>( this, clone ) );
	}

	return clone;
}

// add all standard plugs, not counting optional or auto-duplicating
// Add default plugs (not 'spare' ones')
void ug_base::AddFixedPlugs()
{
	// ug_oversampler_in/out have NULL moduletype.
	if( moduleType )
	{
		// Add standard plugs.
		if( (moduleType->GetFlags() & CF_OLD_STYLE_LISTINTERFACE) != 0 ) // Old internal, SDK1, and SDK2 modules. May have pointers to plug variables.
		{
			SafeInterfaceObjectArray pins;	// retrieve this module's in/out variable addresses.
			ListInterface2( pins );

			for( auto p : pins )
			{
#if _DEBUG
				if (p->isCustomisable(0))
				{
					// Theory: IO_CUSTOMISABLE are *always* IO_AUTODUPLICATE
					assert(p->autoDuplicate(0));
				}
#endif				
				if( (!p->isUiPlug(0) || p->isDualUiPlug(0) ) && /*!p->isCusto misable(0) &&*/ !p->autoDuplicate(0))
				{
					auto up = new UPlug( this, *p );
					AddPlug( up );
				}
			}
		}
		else	// Container, new-style internal modules, and SDK3.
		{
			for( auto& pmap : moduleType->plugs )
			{
				auto p = pmap.second;

#if _DEBUG
				if (p->isCustomisable(0))
				{
					// Theory: IO_CUSTOMISABLE are *always* IO_AUTODUPLICATE
					assert(p->autoDuplicate(0));
				}
#endif				
				if( (!p->isUiPlug(0) || p->isDualUiPlug(0) ) && /*!p->isCust omisable(0) &&*/ !p->autoDuplicate(0))
				{
					auto up = new UPlug( this, *p );
					assignPlugVariable( p->getPlugDescID(0), up );
					AddPlug( up );
				}
			}
		}
	}
}

void ug_base::SetupWithoutCug()
{
	cpuParent = ParentContainer();
	// assign a handle (negative to indicate no coresponding CUG object)
	ParentContainer()->AudioMaster()->AssignTemporaryHandle( this );
	SetAudioMaster( ParentContainer()->AudioMaster() );
	patch_control_container = ParentContainer()->patch_control_container;
	// Add Fixed Plugs.
	bool old_style_listinterface = (moduleType->GetFlags() & CF_OLD_STYLE_LISTINTERFACE) != 0;
	// retrieve this module's in/out variable addresses
	SafeInterfaceObjectArray iobs;

	if( old_style_listinterface ) // needed for MIDI Automator and other old stuff.
	{
		ListInterface2( iobs );
	}

	int plugCount = moduleType->PlugCount();

	for( int j = 0; j < plugCount ; ++j )
	{
		InterfaceObject* pinDescription;

		if( old_style_listinterface )
		{
			pinDescription = iobs[j]; //  contains io variable address specific to this instance.
		}
		else
		{
			pinDescription = moduleType->getPinDescriptionByPosition(j);
			assert( pinDescription->GetVarAddr() == 0 ); // should not contain var address (if so it's bogus).
		}

		// Skip autoduplicate, for vst_in/out on waves.
		if( pinDescription->GetFlags() & IO_AUTODUPLICATE )
		{
			continue;
		}

		// NOTE: This fails to assign plug variable, like AddFixedPlugs() does. probably an oversight. !!!
		AddPlug(new UPlug(this, *pinDescription));
	}

	// create audio plug buffers.
	SetupDynamicPlugs();

	HookUpParameters();
}

void ug_base::Setup( ISeAudioMaster* am, TiXmlElement* xml )
{
	SetAudioMaster( am );
	int handle;
	xml->QueryIntAttribute("Id", &handle);
	AudioMaster()->RegisterDspMsgHandle( this, handle );

#if defined( _DEBUG )
	debug_name = Utf8ToWstring( xml->Attribute("DebugName") );
#endif

	AddFixedPlugs();

	// Was done after creation of plugs previously, but before setting plug defaults.
	// With XML, both creation and setdefault done at same time, so will need to fix any old modules that relied on old behaviour (logic gates).
	// moved back SetupDynamicPlugs();

	// Needs to be after plugs created but before anything connected (because helper often proxies plugs).
	BuildHelperModule();

	// Create IO and autoduplicating Plugs. Set defaults.
	TiXmlElement* plugsElement = xml->FirstChildElement("Plugs");

	std::vector< std::pair<int, std::string> > plugDefaults;

	if( plugsElement )
	{
		for( TiXmlElement* plugElement = plugsElement->FirstChildElement(); plugElement; plugElement=plugElement->NextSiblingElement())
		{
			assert( strcmp(plugElement->Value(), "Plug" ) == 0 );

			// IO Plug on Container or I/O Mod. Identified by "Direction" element.
			int direction = -1;
			if( plugElement->QueryIntAttribute("Direction", &direction) == TIXML_SUCCESS )
			{
				int datatype = -1;
				plugElement->QueryIntAttribute("Datatype", &datatype);
				UPlug* up = new UPlug( this, (EDirection)direction, (EPlugDataType)datatype );
				up->setUniqueId(0); // 'Spare' plug unique ID.
				AddPlug( up );

				// Default on un-connected Container inputs.
				const char* d = plugElement->Attribute("Default");

				// failed any defaults on an oversampler (no parent container):	if (d && parent_container) // fix crash if plugin 'top' container has pin defaults set in XML.
				if (d)
				{
					up->SetDefault( d );
				}

				if( (GetFlags() & UGF_IO_MOD) != 0 )
				{
					int tiedToModule = -1;
					int tiedToPlugIdx = -1;
					plugElement->QueryIntAttribute("TiedTo", &tiedToModule);
					plugElement->QueryIntAttribute("TiedToPinIdx", &tiedToPlugIdx);

					ug_base* tiedToContainer = dynamic_cast<ug_base*>( AudioMaster()->HandleToObject( tiedToModule ) );
					UPlug* p2 = tiedToContainer->GetPlug( tiedToPlugIdx );
					// each IO_UG should link to it's parent container.
					up->TiedTo = p2;
					p2->TiedTo = up;
				}
				else
				{
					bool isCv{};
					plugElement->QueryBoolAttribute("CV", &isCv);
					if (isCv)
					{
						up->SetFlag(PF_CV_HINT);
					}
				}
			}
			else
			{
				// Autoduplicating plug.
				int autoCopyDescId = -1;
				
				if (plugElement->QueryIntAttribute("AutoCopy", &autoCopyDescId) == TIXML_SUCCESS)
				{
					auto pinDescription = moduleType->getPinDescriptionById(autoCopyDescId);
					if (pinDescription)
					{
						assert(pinDescription->autoDuplicate(0));
/*
* ??? STUPID ???
* seem to be misguidely changing id to index (then using id when we should be using index)
						UPlug* up = new UPlug(this, *pinDescription);
						int uniqueId = 0;
						plugElement->QueryIntAttribute("Id", &uniqueId);
						up->setUniqueId(uniqueId);
*/
						AddPlug(new UPlug(this, *pinDescription));
					}
				}

				// Default (only if no Parameter attached).
				auto d = plugElement->Attribute("Default");

				if (d)
				{
					if (parent_container) // fix crash if plugin 'top' container has pin defaults set in XML.
					{
						int idx = 0; // default is zero.
						plugElement->QueryIntAttribute("Idx", &idx);

						plugDefaults.push_back(std::pair<int, std::string>(idx, std::string(d)));
					}
				}
				else
				{
					// settable outputs MUST be set here, else crashes when trying to clone fixed buffers.
					assert(getModuleType()->UniqueId() != L"Fixed Values");
				}
			}
		}
	}

	SetupDynamicPlugs();

	for (auto& it : plugDefaults)
	{
		GetPlug(it.first)->SetDefault(it.second.c_str());
	}

	HookUpParameters();

	int useDebugger = -1;
	if (xml->QueryIntAttribute("Debugger", &useDebugger) == TIXML_SUCCESS)
	{
		AttachDebugger();
	}

	// dones earlier so SetDefault works on output plugs	SetupDynamicPlugs(); // Allocate audio buffers.
}

void ug_base::HookUpParameters()
{
	if (getModuleType() == nullptr) // Oversampler, no parameters anyhow.
		return;

	// Hook up Parameters.
	int dspIdx = 0;
	for (auto pmap : getModuleType()->plugs)
	{
		auto pinfo = pmap.second;

		if (!pinfo->isUiPlug(nullptr) || pinfo->isDualUiPlug(nullptr)) // DSP pins.
		{
			// Special case for host-controlled legacy module pins ( ug_patch_param_watcher, voice-mute only )
			if (pinfo->isHostControlledPlug(0))
			{
				assert(pinfo->getHostConnect(0) != HC_NONE); // older method relyed on StringToHostControl(p->getName()); should be fixed already in module_info.

				auto pin = GetPlug(dspIdx);
				if (pin->Direction == DR_OUT)
				{
					// Still using old dodgy method. TODO: Update as below.
					ParentContainer()->GetParameterWatcher()->CreatePlug(pinfo->getHostConnect(0), pin);
				}
				else
				{
					// problem. Parameter setter creates plugin, then tries to hook up to it's patch-manager,
					// however if parameter can't be found, defers to outermost patch-manager, which is not
					// gaurenteed to be in correct sort-order relative to a nested pp-setter (only it's own one).
					// ParentContainer()->GetParameterSetter()->CreatePlug(pinfo->getHostConnect(0), pin);

					ParentContainer()->ConnectHostControl(pinfo->getHostConnect(0), pin);
				}
			}

			// handle parameter plugs.
			if (pinfo->isParameterPlug(0))
			{
				/* should now be handled by module_info
				if (pinfo->getParameterId(0) < 0)
					pinfo->setParameterId(0); // assuming SDK1 modules use only one parameter max.
				*/
				assert(pinfo->getParameterId(0) >= 0);

				// Connect Parameter.
				auto pin = GetPlug(dspIdx);
				assert(!pin->InUse());
				assert(pin->getPlugIndex() == dspIdx);

				if (pin->Direction == DR_OUT)
				{
					ParentContainer()->GetParameterWatcher()->CreateParameterPlug(Handle(), pinfo->getParameterId(0), pin);
				}
				else
				{
					ParentContainer()->GetParameterSetter()->ConnectParameter(Handle(), pinfo->getParameterId(0), pin);
				}
			}
			++dspIdx;
		}
	}
}

void ug_base::SetupClone( ug_base* clone )
{
	// create linked list of clones
	// add to tail.
	ug_base* lastClone = this;

	while( lastClone->m_next_clone )
	{
		lastClone = lastClone->m_next_clone;
	}

	clone->m_clone_of = this;
	lastClone->m_next_clone = clone;
	clone->moduleType = moduleType;
	clone->m_handle = m_handle;
	clone->latencySamples = latencySamples;
	clone->parent_container = parent_container;
	clone->cpuParent = cpuParent;
	clone->patch_control_container = patch_control_container;
	clone->SetAudioMaster( EventProcessor::AudioMaster() );
	clone->AddPlugs(this);
	clone->flags = flags;
	clone->SetPolyphonic( GetPolyphonic() ); // copy polyphonic flag (allows proper suspend/resume)
	clone->SetFlag( UGF_OPEN|UGF_SUSPENDED, false ); // ain't open yet

	if( m_debugger )
	{
		clone->m_debugger = std::make_unique<UgDebugInfo>(clone);
	}

#if defined( _DEBUG )
	clone->debug_name = debug_name;
#endif
}

IDspPatchManager* ug_base::get_patch_manager()
{
	return ParentContainer()->get_patch_manager();
}

void ug_base::onSetPin(timestamp_t /*p_clock*/, UPlug* /*p_to_plug*/, state_type /*p_state*/)
{
}

// handles setting a control-rate pin and also MIDI.
// called by TransmitPinValue(), midi_output::Send().
void ug_base::SetPinValue(timestamp_t timestamp, int pin_index, int datatype, const void* data1, int data_size1)
{
	if( IsSuspended() )
	{
		if( datatype != DT_MIDI )
		{
			DiscardOldPinEvents( pin_index, datatype);
		}
	}

	int32_t raw_output_value[2] = {};
	char* extraData = 0;

	if( data_size1 <= sizeof(raw_output_value) )
	{
		if( data_size1 > 0 )
		{
			unsigned char* dest = (unsigned char*)raw_output_value;
			unsigned char* src = (unsigned char*) data1;

			for(int i = 0 ; i < data_size1; i++ )
			{
				*dest++ = *src++;
			}
		}
	}
	else
	{
		extraData = AudioMaster()->AllocateExtraData(data_size1);

        assert( extraData != 0 ); // Out of memory?
		memcpy( extraData, data1, data_size1 );
	}

	// TODO each UPlug to record plug_id (not nesc same as index) !!!
	// no not when asleep...assert( SampleClock() == AudioMaster()->BlockStartClock() );
	// todo: all timestamps block-relative
	//timestamp_t timeStamp = block_relative_clock + AudioMaster()->BlockStartClock();
	int eventType;

	switch(datatype)
	{
	case DT_MIDI:
		eventType = UET_EVENT_MIDI;
		break;

	case DT_BLOB2:
	{
		// blob2 data represents a pointer to a reference counted object.
		// need to increment the count to indicate that the data is 'in flight' and can't be reused yet.
		auto blob2 = reinterpret_cast<gmpi::IMpUnknown**>(const_cast<void*>(data1));
		if(*blob2)
			(*blob2)->addRef();
	}
	[[fallthrough]];

	default:
		eventType = UET_EVENT_SETPIN;
		break;
	}

	AddEvent(
	    new_SynthEditEventB(
			timestamp,
			eventType,
			pin_index,				// int 1	- pin Index
			data_size1,				// int 2	- data size in bytes
			raw_output_value[0],	// parm3	- data (if size <= 8 bytes)
			raw_output_value[1],	// parm4	- voice or high word of data
			extraData ),			// extra	- data (if size > 8 bytes)
		datatype
	);
}

// for older modules. Uses pins value as stored in io_variable.
// SDK3 don't use io_variable.
void ug_base::SendPinsCurrentValue( timestamp_t timestamp, UPlug* from_plug )
{
	// need to record output value at time of event (may change later).
	int32_t data_size = 0; // size in bytes of pin value.
	void* data = from_plug->io_variable;
	int cast_output_value; // for short and bool convert to int.

	switch( from_plug->DataType )
	{
	case DT_ENUM:
		{
		    cast_output_value = (int) *((short*)from_plug->io_variable);
		    data_size = sizeof(int);
		    data = &cast_output_value;
		}
		break;

	case DT_INT:
		{
		    //cast_output_value = *((int *)from_plug->io_variable);
		    data_size = sizeof(int);
		}
		break;

	case DT_BOOL:
		{
		    cast_output_value = (int) *((bool*)from_plug->io_variable);
		    data_size = sizeof(int);
		    data = &cast_output_value;
		}
		break;

	case DT_FLOAT:
		{
		    //cast_output_value = (int) *((int *)from_plug->io_variable);
		    data_size = sizeof(float);
		}
		break;

	case DT_DOUBLE:
		{
		    data_size = sizeof(double);
		}
		break;

	case DT_BLOB:
	{
		MpBlob* b = (MpBlob*)from_plug->io_variable;
		data_size = b->getSize();
		data = b->getData();
	}
	break;

	case DT_BLOB2:
	{
		data_size = sizeof(gmpi::ISharedBlob*);
	}
	break;

	case DT_TEXT:
		{
		    std::wstring* txt = (std::wstring*)from_plug->io_variable;
		    data_size = (int) (txt->size() * sizeof(wchar_t));
		    data = (void*) txt->data();
		}
		break;

	case DT_STRING_UTF8:
		{
		    std::string* txt = (std::string*)from_plug->io_variable;
		    data_size = (int) txt->size();
		    data = (void*) txt->data();
		}
		break;

	case DT_MIDI2:
	case DT_FSAMPLE:
	default:
		assert(false);
	break;
	}

	assert( data || data_size == 0 ); // Blobs can transmit 'blank' data.
	from_plug->Transmit( timestamp, data_size, data );
}

void ug_base::Resume()
{
	if( (flags & UGF_SUSPENDED) == 0 )
	{
//		assert( (flags & UGF_NEVER_SUSPEND) != 0 );
		return;
	}

	assert( (flags & UGF_SUSPENDED) != 0 );

	SetFlag( UGF_SUSPENDED, false );

	// if events have arrived since suspend, need to wake
	if( sleeping )
	{
		if( events.empty() )
		{
			return;
		}
		Wake();
	}
	else	// back on list of block devices, if not sleeping (also sets sampleclock)
	{
		AudioMaster()->InsertBlockDev(this);
	}

	// if voice suspends/resumes, ug will be at random block pos,
	// so static_output_count needs resetting
	if( static_output_count > 0 )
	{
		ResetStaticOutput();
	}
}

// Create a copy of annother's connectors (for polyphonic sound)
void ug_base::CloneConnectorsFrom( ug_base* FromUG, CUGLookupList& UGLookupList )
{
	assert( this != FromUG ); // assumes 'this' is a clone

	for( auto p : FromUG->plugs )
	{
		if( p->Direction == DR_OUT ) // Outgoing connection (always clone)
		{
			// very important to traverse list in reverse.
			// because if an adder is inserted, the connector will be deleted, then re-added to list tail
			// , we don't want to clone the connector twice!
			for (int i = static_cast<int>(p->connections.size()) - 1 ; i >= 0 ; --i ) // must be int to satisfy end condition (less than zero).
			{
				auto to = p->connections[i];

				// find 'to' ug's duplicate
				auto clone_of_to_ug = UGLookupList.LookupPolyphonic(to->UG);
				if( clone_of_to_ug )
				{
					connect(plugs[p->getPlugIndex()], clone_of_to_ug->plugs[to->getPlugIndex()]);
				}
				else
				{
					_RPT0(_CRT_WARN, "LookupPolyphonic FAIL ");
					DebugIdentify(true);
					_RPT0(_CRT_WARN, " -> ");
					to->UG->DebugIdentify(true);
					_RPT0(_CRT_WARN, "\n");
					assert(FromUG->GetFlag(UGF_POLYPHONIC_GENERATOR_CLONED) != 0 && to->DataType == DT_MIDI2); // MIDI-CV have dummy connection to clones. Don't need cloning obviously.
				}
			}
		}
		else // incoming connection (clone if not coming from another clone(poly) module)
		{
			for( auto it2 = p->connections.rbegin(); it2 != p->connections.rend(); ++it2 )
			{
				UPlug* from = *it2;
				if( from->UG->GetPolyphonic() == false )
				{
					// reverse connection
					// inefficient getting from plug this way, we already know it...		connect( con_from_ug->plugs[c->From->getPlugIndex()], plugs[c->To->getPlugIndex()] );
					assert( from->UG->plugs[from->getPlugIndex()] == from ); // check refactoring below correct
					connect( from, plugs[p->getPlugIndex()] );
				}
			}
		}
	}

	//	_RPT0(_CRT_WARN, "---------------------------------------------------) \n" );
}

void ug_base::CopyConnectorsFrom( ug_base* FromUG, CUGLookupList& UGLookupList )
{
	assert( this != FromUG ); // assumes 'this' is a clone

	for( vector<UPlug*>::iterator it = FromUG->plugs.begin() ; it != FromUG->plugs.end() ; ++it )
	{
		UPlug* p = *it;
		if( p->Direction == DR_OUT )
		{
			// very important to traverse list in reverse.
			// because if an adder is inserted, the connector will be deleted, then re-added to list tail
			// , we don't want to clone the connector twice!
			for( vector<UPlug*>::reverse_iterator it2 = p->connections.rbegin(); it2 != p->connections.rend(); ++it2 )
			{
				UPlug* to = ( *it2 );
				ug_base* con_to_ug = ( *it2 )->UG;
//				if( con_from_ug == FromUG ) // Outgoing connection.
				{
					// find 'to' ug's duplicate
					ug_base* clone_of_to_ug = UGLookupList.Lookup2(con_to_ug);
					assert(clone_of_to_ug);
					UPlug* to_plug = clone_of_to_ug->plugs[to->getPlugIndex()];
					assert(!to_plug->InUse() || to_plug->DataType == DT_MIDI2);
					connect(plugs[p->getPlugIndex()], to_plug);
				}
			}
		}
	}

	//	_RPT0(_CRT_WARN, "---------------------------------------------------) \n" );
}
// return either CUG title, or if not avail, class name
std::wstring ug_base::DebugModuleName( bool andHandle )
{
	wstring res;
	{
		if (moduleType)
		{
			res = moduleType->GetName();
			if (res.empty())
			{
				res = moduleType->UniqueId();
			}
		}
		else
		{
			res = Utf8ToWstring(typeid(*this).name());

			if (Left(res, 5) == L"class")
				res = Right(res, res.size() - 6);
		}

#if defined( _DEBUG )

//#if defined( SE_EDIT _SUPPORT )
//		AFX_MANAGE_STATE(AfxGetStaticModuleState());
//#endif

		if (!debug_name.empty() && debug_name != res)
		{
			res = res + L" \"" + debug_name + L"\"";
		}
#endif
	}

	if( andHandle )
	{
		res += L"[";
		res += IntToString( Handle() );
		res += L"]";
	}

	return res;
}

// return either CUG title, or if not avail, class name
std::wstring ug_base::GetFullPath()
{
	std::wstring path;
	ug_base* m = this;

	while( m )
	{
		//if( m->Get CUG() )
		//{
			path = m->DebugModuleName() + L"/" + path;
		//}
		//else
		//{
		//	std::wstring temp = To Wstring(typeid(*m).name());

		//	if( Left(temp,5) == L"class" )
		//		temp = Right(temp, temp.size() - 6 );

		//	path = temp + L"/" + path;
		//}

		m = m->ParentContainer();
	}

	return path;
}

#if defined( _DEBUG )
void ug_base::DebugPrintName(bool withHandle)
{
	int modulesOfInterest[] = { -39, -261,723353632 };

	bool highlight = std::find(std::begin(modulesOfInterest), std::end(modulesOfInterest), Handle()) != std::end(modulesOfInterest);
	if (highlight)
	{
		_RPT0(_CRT_WARN, "** ");
	}

	_RPTW1(_CRT_WARN, L"%s ", DebugModuleName().c_str() );
	
	if (withHandle)
	{
		_RPTW1(_CRT_WARN, L"[%d] ", Handle());
	}

	auto a = dynamic_cast<ug_adder2*>( this );
	if( a && !a->plugs.empty() && !a->plugs[0]->connections.empty() )
	{
		auto con_to = a->plugs[0]->connections.front()->UG;
		_RPTW1(_CRT_WARN, L"connected to %s ", con_to->DebugModuleName().c_str() );

		if (withHandle)
		{
			_RPTW1(_CRT_WARN, L"[%d] ", con_to->Handle());
		}
	}
}

void ug_base::DebugPrintContainer(bool withHandle)
{
	if( parent_container )
	{
		_RPTW1(_CRT_WARN, L" in %s", parent_container->DebugModuleName().c_str() );
		if (withHandle)
		{
			_RPTW1(_CRT_WARN, L"[%d]", parent_container->Handle());
		}
		_RPT0(_CRT_WARN, " ");
	}
	else
	{
		_RPT0(_CRT_WARN, " with no container " );
	}
}

void ug_base::DebugIdentify(bool withHandle)
{
	DebugPrintName(withHandle);
	DebugPrintContainer(withHandle);
}

void ug_base::DebugVerfifyEventOrdering()
{
	timestamp_t last_timestamp = 0;
	int last_stat_change_pin = -1;
	SynthEditEvent* prev = 0;

	for( EventIterator it(events.begin()) ; it != events.end() ; ++it )
	{
		SynthEditEvent* e = *it;
		assert( e->timeStamp >= last_timestamp );

		if( e->eventType == UET_EVENT_SETPIN /*UET _STAT_CHANGE*/ && prev )
		{
			//			_RPT3(_CRT_WARN, "sc=%5d ug=%x event type %d\n", e->timeStamp, this, e->eventType );
			int pin_number = e->parm1; //((conn ector*) e->parm3)->To->getPlugIndex();
			assert( prev->timeStamp < e->timeStamp || last_stat_change_pin < pin_number || prev->parm3 != e->parm3 );
			//			_RPT3(_CRT_WARN, "sc=%5d ug=%x STAT CHANGE pin %d\n", e->timeStamp, this, pin_number );
			last_stat_change_pin = pin_number;
			prev = e;
		}
	}
}

#endif

void ug_base::CreateSharedLookup2( const std::wstring& id, ULookup * & lookup_ptr, int sample_rate, size_t p_size, bool create, SharedLookupScope scope)
{
	AudioMaster()->ugCreateSharedLookup( id, (LookupTable**) &lookup_ptr, sample_rate, p_size, false, create, scope );
}

void ug_base::CreateSharedLookup2( const std::wstring& id, ULookup_Integer * & lookup_ptr, int sample_rate, size_t p_size, bool create, SharedLookupScope scope)
{
	AudioMaster()->ugCreateSharedLookup( id, (LookupTable**) &lookup_ptr, sample_rate, p_size, true, create, scope );
}


int32_t ug_base::allocateSharedMemory( const wchar_t* table_name, void** table_pointer, float sample_rate, int32_t size_in_bytes, int32_t& ret_need_initialise, int32_t visibilityScope )
{
	ret_need_initialise = false;
	ULookup* lu = 0;
	*table_pointer = 0;
	int size_in_floats = (size_in_bytes+3) / 4;

	// Size <= 0 indicates not to create it, just locate existing.
	//CreateSharedLookup2( table_name, lu, (int) sample_rate, size_in_floats, size_in_bytes > 0, SLS_ONE_MODULE );
	AudioMaster()->ugCreateSharedLookup( table_name, (LookupTable**) &lu, (int) sample_rate, size_in_floats, false, size_in_bytes > 0, (SharedLookupScope) visibilityScope );

	if( lu == 0 )
	{
		// OK to 'fail' if only a query (size_in_floats == -1 ).
		return size_in_floats != -1 ? gmpi::MP_FAIL : gmpi::MP_OK;
	}

	// note: this must be checked before SetInitialised()
	ret_need_initialise = !lu->GetInitialised();

	if( !lu->GetInitialised() )
	{
#if 0 //defined( _DEBUG )
		if( size_in_bytes > 0x100000 )
		{
			_RPT1(_CRT_WARN, "allocateSharedMemory %.1f MB\n", size_in_bytes / (float) 0x100000 );
		}
		else
		{
			_RPT1(_CRT_WARN, "allocateSharedMemory %.1f kB\n", size_in_bytes / (float) 0x400 );
		}
#endif
		lu->SetInitialised();
	}

	assert( lu->table != 0 );
	*table_pointer = lu->table;

	return gmpi::MP_OK;
}

// clone the plugs of another
void ug_base::AddPlugs(ug_base* copy_ug)
{
	// first add standard plugs.
	// inits io variables on older modules. does nothing on SDK3 modules.
	AddFixedPlugs();

	// then copy any dynamic plugs. (SDK3 relies on this to add all plugs)
	for( unsigned int x = (unsigned int) plugs.size(); x < copy_ug->plugs.size() ; x++ )
	{
		AddPlug( new UPlug( this, copy_ug->plugs[x] ) );
	}

	// create plug variables.
	SetupDynamicPlugs();
	// now check we did it right
	assert( plugs.size() == copy_ug->plugs.size() );
}

void ug_base::SetPPVoiceNumber(int n)
{
	SetPolyphonic(true);
	pp_voice_num = n;
}

bool ug_base::PPGetActiveFlag()
{
	// check for 'Active' plugs that are downstream
	// An 'Active' plug can't combine voices.
	// eg osc freq, you can't add several note pitchs into an osc
	// and get the same result as feeding each into a seperate osc and adding outputs
	for( auto p : plugs )
	{
		if( p->Direction == DR_IN )
		{
			if( p->GetPPDownstreamFlag() && p->PPGetActiveFlag() )
				return true;
		}
	}

	return false;
}

// Polyphonic determination helper routines
struct FeedbackTrace* ug_base::PPSetDownstream()
{
	if(GetPPDownstreamFlag())
	{
		return nullptr;
	}

	if(GetPolyAgregate()) // Means we've reached an I/O Mod, Wave-Out etc, which forces end of polyphonic chain.
	{
		SetFlag(UGF_PP_TERMINATED_PATH);
		return nullptr;
	}

	SetPPDownstreamFlag(true);

#ifdef _DEBUG
//	const auto polyContainer = parent_container->getVoiceControlContainer();
#endif

	bool hasAudioOutputs = false;
	for( auto p : plugs )
	{
		if( p->Direction == DR_OUT && p->DataType != DT_MIDI )
		{
			hasAudioOutputs = true;
			for (int j = 0; j < p->connections.size(); j++) // iterate by index because ug_poly_to_monoB can insert additional connectors during this loop.
			{
				auto to = p->connections[j];
				if( to->UG->GetFlag(UGF_POLYPHONIC_GENERATOR) )
				{
					auto e = new FeedbackTrace(SE_FEEDBACK_TO_NOTESOURCE);
					e->AddLine(p, to);
					return e;
				}

				auto r = to->PPSetDownstream();
				if (r != 0)
				{
					r->AddLine(p, to);
					return r;
				}

				if(to->UG->GetFlag(UGF_PP_TERMINATED_PATH))
				{
					SetFlag(UGF_PP_TERMINATED_PATH);
				}
			}
		}
	}

	// When we get to a module that is end of audio flow, consider that a 'real' output path
	// that needs monitoring (not a hanging module).
	if(!hasAudioOutputs)
	{
		SetFlag(UGF_PP_TERMINATED_PATH);
	}

	return nullptr;
}

// Used by drum trigger to assign voice number to each module
// depending on what drum it is playing
// voice number starts at -1, and is set to either a drum voice number, or zero if it's shared
void ug_base::PPPropogateVoiceNum(int n)
{
	// already done for this?
	if( pp_voice_num == 0 || pp_voice_num == n || GetPolyAgregate() )
	{
		return;
	}
	else
	{
		// already assigned to another voice, if so assign to voice 0 (mono/shared)
		if( pp_voice_num > 0 )
		{
			pp_voice_num = 0;
		}
		else
		{
			pp_voice_num = n;
		}

#ifdef _DEBUG

		if( SeAudioMaster::GetDebugFlag( DBF_TRACE_POLY ) )
		{
			DebugPrintName(true);
			DebugPrintContainer();
			_RPT1(_CRT_WARN, "-PropogateVoiceNum (%d)\n",pp_voice_num );
		}

#endif

		// continue propagation
		// For each plug...
		for( vector<UPlug*>::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
		{
			UPlug* p = *it;

			if( p->Direction == DR_OUT )
			{
				for( auto it2 = p->connections.begin(); it != p->connections.end(); ++it )
				{
					ug_base* downstream = (*it2)->UG;
					// if user has done weird stuff (puttin MIDI automators in sub container of drum module)
					// then prevent voice propogation going into sub-container
					// else sub-container generates assertion error when it finds modules tagged as voice 1 in voice zero
					if( downstream->parent_container == parent_container )
					{
						downstream->PPPropogateVoiceNum( n );
					}
				}
			}
		}
	}
}

void ug_base::FlagUpStream(int flag, bool debugTrace)
{
	// Avoid getting stuck in feedback loops.
	if (GetFlag(flag))
	{
		return;
	}
	SetFlag(flag);

#ifdef _DEBUG

	if (debugTrace)
	{
		DebugPrintName();
		DebugPrintContainer();
		_RPT1(_CRT_WARN, "- Setflag %x\n", flag);
	}

#endif

	// For each plug...
	for (auto p : plugs)
	{
		if (p->Direction == DR_IN)
		{
			// for each line
			for( auto fromPlug : p->connections )
			{
				auto ug = fromPlug->UG;

				// Avoid flagging modules outside the patch-automators container, they will have their own patch-automator
				if (ug->patch_control_container != patch_control_container)
					continue;

				if (ug->GetFlag(UGF_PARAMETER_SETTER) && flag == UGF_UPSTREAM_PARAMETER) // ignore snapshot upstream flagging.
				{
					// Re-route to secondary pp-setter.
					auto pps = dynamic_cast<ug_patch_param_setter*>(ug);
					assert(p->connections.size() == 1);
					pps->TransferConnectiontoSecondary(fromPlug);
					break; // next plug.
				}

				ug->FlagUpStream(flag, debugTrace);
			}
		}
	}
}

void ug_base::PPSetUpstream()
{
	if( !GetPPDownstreamFlag() || GetPolyphonic() )
		return;

	SetPolyphonic(true);

	// For each plug...
	for( auto p : plugs )
	{
		if( p->Direction == DR_IN )
		{
			for( auto from : p->connections )
			{
				from->UG->PPSetUpstream();
			}
		}
	}
}

// For Drums.
void ug_base::PPPropogateVoiceNumUp(int n)
{
	// already done for this?
	if( pp_voice_num == 0 || pp_voice_num == n || GetPolyAgregate() )
	{
		return;
	}
	else
	{
		// already assigned to another voice, if so assign to voice 0
		if( pp_voice_num > 0 )
		{
			pp_voice_num = 0;
		}
		else
		{
			pp_voice_num = n;
		}

		// continue propagation
		// For each plug...
		for( vector<UPlug*>::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
		{
			UPlug* p = *it;

			if( p->Direction == DR_IN )
			{
				// for each line
				for( auto& from : p->connections)
				{
					// prevent midi_autoamtor's midi links to controls from assigning it to any voice other than zero.
					// if it's assigned to non-zero voice, it become polyphonic and gets suspended, which causes it to crash
					// when it receives midi.
					if( from->DataType != DT_MIDI2 )
					{
						from->UG->PPPropogateVoiceNumUp(n);
					}
				}
			}
		}
	}
}

// Auto Note Shutdown helper
// once the last unit gen in the voice's chain stops, the entire voice can be shutdown
// this routine inserts adders if this ug is polyphonic and one or more after it are not
// i.e this is last polyphonic
// synths need adders at end of each voice chain, unless they are monophonic
// drum modules don't ( because voice is never cloned )
void ug_base::InsertVoiceAdders()
{
	if( !GetPolyphonic() || ( GetFlags() & UGF_VOICE_MUTE ) != 0 )
		return;

	for( auto it = plugs.begin() ; it != plugs.end() ; ++it )
	{
		UPlug* p = *it;

		if( p->Direction == DR_OUT && p->DataType != DT_MIDI ) // Being event-based, MIDI is automatically merged.
		{
			// iterate connectors in REVERSE to remove probs if need to delete one
			for( int i = (int)p->connections.size() - 1; i >= 0; --i )
			{
				// ug_patch_param_watcher has MIDI connection to ug _patch_automator_out. That's OK poly->mono.
				UPlug* to_plug = p->connections[i];

				if( ! to_plug->UG->GetPolyphonic() )
				{
					// can't agregate non-audio polyphonic signals.
					assert( to_plug->DataType == DT_FSAMPLE );
					p->DeleteConnection(to_plug);

					UPlug* from_plug;

					if(!to_plug->UG->IgnoredByVoiceMonitor())
					{
						// insert voice-mute (smoothly fades out stolen voices)
						Module_Info* voiceMuteType = ModuleFactory()->GetById( L"SE Voice Mute" );

						ug_base* voiceMute = voiceMuteType->BuildSynthOb();
						parent_container->AudioMaster()->AssignTemporaryHandle( voiceMute );
						voiceMute->SetPolyphonic(true);
						voiceMute->SetFlag( UGF_VOICE_MUTE );
						if(to_plug->UG->GetFlag(UGF_PP_TERMINATED_PATH))
						{
							voiceMute->SetFlag( UGF_PP_TERMINATED_PATH );
						}
						parent_container->AddUG( voiceMute );
						voiceMute->SetupWithoutCug();

						// MIDI-CV etc delay gate by 3 samples, results in garbage playing for those 3 samples, need
						// VoiceMute to stay quiet for that time.
//						if( (ParentContainer()->front()->NoteSource->GetFlags() & UGF_DELAYED_GATE_NOTESOURCE) != 0 )
						if(	ParentContainer()->isContainerPolyphonicDelayedGate() )
						{
							voiceMute->GetPlug(3)->SetDefault( "1" );
						}

						connect( p, voiceMute->plugs[0] );
						from_plug = voiceMute->plugs[2];
#ifdef _DEBUG
						if( SeAudioMaster::GetDebugFlag( DBF_TRACE_POLY ) )
						{
							DebugPrintName(true);
							_RPT0(_CRT_WARN, "-Last PolyPhonic, Voice-Mute Attached.\n" );
						}

						voiceMute->debug_name = L"VoiceMute from ";
						voiceMute->debug_name += p->UG->DebugModuleName();
						voiceMute->debug_name += L"[";
						voiceMute->debug_name += IntToString(p->UG->Handle());
						voiceMute->debug_name += L"]";
						voiceMute->debug_name += L".pin";
						voiceMute->debug_name += IntToString(p->getPlugIndex());
#endif
					}
					else
					{
						from_plug = p;
					}

					// ensure tail ug feeds adder (for voice aggregation)
					// ensure all connections between poly and non-poly
					// ug's have an adder (for voice aggregation)
					// if not add one.
					if( to_plug->IsAdder() )
					{
						// if last ug is an adder, to_plug's proxie will already be set,
						// if so NULL it so I don't inadvertantly connect to wrong adder
						to_plug->Proxy = NULL;
						connect( from_plug, to_plug );
						// drum trigger needs to know that voice chain ends here
						to_plug->UG->SetFlag(UGF_POLYPHONIC_AGREGATOR|UGF_PP_TERMINATED_PATH);
					}
					else
					{
						//_RPT0(_CRT_WARN, " Adder inserted in cont\n" );
						// Insert an adder or MIDI adder ((!! hm midi is not polyphonic, why an adder???)
						ug_base* adder = parent_container->InsertAdder( p->DataType );
						// connect adder input to this. divertes to p->proxy
						// ( the UG inside the cont that was con to p)
						//_RPT2(_CRT_WARN, "] Connecting %s plug %d to adder\n", p->UG->DebugModuleName(),p->getPlugIndex() );
						// connect( p, adder->plugs[1] );
						connect( from_plug, adder->plugs[1] );
						//_RPT2(_CRT_WARN, "] Connecting adder to %s plug %d\n", to_plug->UG->DebugModuleName(),to_plug->getPlugIndex() );
						//						connect( p, to_plug );
						// if last ug is an adder, to_plug's proxie will already be set,
						// if so NULL it so I don't inadvertantly connect to wrong adder
						to_plug->Proxy = NULL;
						connect( adder->plugs[0], to_plug );

						// hack for scope
						if( to_plug->UG->IgnoredByVoiceMonitor() )
						{
							adder->SetFlag(UGF_VOICE_MON_IGNORE);
						}

						to_plug->Proxy = adder->plugs[1];
						// drum trigger needs to know that voice chain ends here
						adder->SetFlag(UGF_POLYPHONIC_AGREGATOR|UGF_PP_TERMINATED_PATH);
					}
				}
			}
		}
	}
}

// returns true if ug is absolute last in voice chain
// return false if ANY downstream ug is also polyphonic

// NEW behavior: return true if has at least 1 connection to non-polyphonic module
// this works better in patch's with several outputs, (e.g osc->switch->waveshaper->out or osc->switch->out)
bool ug_base::isPolyLast()
{
	return (flags & UGF_DEBUG_POLY_LAST ) != 0;
}

// uncomment to get traces msgs
//#define DEBUG_CONECTIONS

void ug_base::connect_oversampler_safe(UPlug* fromPlug, UPlug* toPlug)
{
	auto from = fromPlug->UG;
	auto to = toPlug->UG;

	// Are source and destination oversampled relative to each other?
	// If so route up to common container.
	if (from->AudioMaster() != to->AudioMaster())
	{
		std::vector<ug_oversampler*> sourceOversamplers;

		auto oversampler = dynamic_cast<ug_oversampler*>(from->AudioMaster());
		while (oversampler)
		{
			sourceOversamplers.push_back(oversampler);
			oversampler = dynamic_cast<ug_oversampler*>(oversampler->AudioMaster());
		}

		std::vector<ug_oversampler*> destOversamplers;
		oversampler = dynamic_cast<ug_oversampler*>(to->AudioMaster());
		while (oversampler)
		{
			destOversamplers.push_back(oversampler);
			oversampler = dynamic_cast<ug_oversampler*>(oversampler->AudioMaster());
		}

		// Discard common oversampling.
		while (!sourceOversamplers.empty() && !destOversamplers.empty() && sourceOversamplers.back() == destOversamplers.back())
		{
			sourceOversamplers.pop_back();
			destOversamplers.pop_back();
		}

		for (auto it = sourceOversamplers.rbegin(); it != sourceOversamplers.rend(); ++it)
		{
			fromPlug = (*it)->routePatchCableOut(fromPlug);
		}

		for (auto it = destOversamplers.rbegin(); it != destOversamplers.rend(); ++it)
		{
			toPlug = (*it)->routePatchCableOut(toPlug);
		}
	}

	connect(fromPlug, toPlug);
}

void ug_base::connect( UPlug* from_plug, UPlug* to_plug )
{
	assert( from_plug->Direction == DR_OUT );
	assert( to_plug->Direction == DR_IN );
#ifdef DEBUG_CONECTIONS
	_RPT0(_CRT_WARN, "\n Connecting " );
	from_plug->UG->DebugPrintName();
	_RPT0(_CRT_WARN, "to " );
	to_plug->UG->DebugPrintName();
	_RPT0(_CRT_WARN, "\n" );
#endif

	// if an input already has an adder
	// Proxy will point to the adder
	while( to_plug->Proxy != NULL )
		to_plug=to_plug->Proxy;

	while( from_plug->Proxy != NULL )
		from_plug=from_plug->Proxy;

	if( to_plug->DataType != from_plug->DataType )
	{
		const wchar_t* datatypes[] = { L"Enum", L"Text", L"Midi", L"Double", L"Bool", L"Volts", L"Float", L"", L"Int", L"Int64", L"Blob", L"class-unused", L"Text8", L"Blob2"};

		wchar_t temp[30];
		swprintf(temp, sizeof(temp)/sizeof(temp[0]), L"SE %lsTo%ls", datatypes[from_plug->DataType], datatypes[to_plug->DataType]);

		// now add a UConverter_SB or UConverter_BS
		const wchar_t* converter_id = temp;
		int converterInputPlug = 0;
		int converterOutputPlug = 1;

		switch( from_plug->DataType )
		{
		case DT_FSAMPLE:
			switch (to_plug->DataType)
			{
			case DT_FLOAT:
				converter_id = L"VoltsToFloat2";
				converterOutputPlug = 3;
				break;

			case DT_ENUM:
				/* don't work without extra help during setup (to get enum list)
				converter_id = L"Voltage To List";
				*/
				return;
				break;
       		default:
                break;
            
			};
			break;

		case DT_INT:
						switch (to_plug->DataType)
						{
			case DT_ENUM:
			/* don't work when created on the fly, because it can't access it's enum list.
				converter_id = L"SE Int To List";
			*/
				return; // some old projects may attempt this.
							break;
              		default:
                break;
                     
						};
						break;

		case DT_FLOAT:
			switch (to_plug->DataType)
			{
			case DT_FSAMPLE:
				converter_id = L"FloatToVolts";
				converterInputPlug = 1;
				converterOutputPlug = 2;
				break;

       		default:
                break;
			};
			break;

		default:
			break;
		};

		assert(converter_id != nullptr);
		auto converterType = ModuleFactory()->GetById(converter_id);

		if (!converterType)
		{
			assert(false); // invalid connection.
			return;
		}
	
		ug_base* converter = nullptr;

		// check if converter already in place.
		for (auto p : from_plug->connections)
		{
			if (p->UG->getModuleType() == converterType)
			{
				converter = p->UG;
				break;
			}
		}

		if (converter == nullptr)
		{
			converter = converterType->BuildSynthOb();
			assert(converter);

			// !!! Should use SetupWithoutUg() , except it currently don't assign plug variable to volts-to-float
			AudioMaster()->AssignTemporaryHandle(converter);
			converter->AddFixedPlugs();
			auto convertersContainer = to_plug->UG->parent_container;
			converter->SetAudioMaster(convertersContainer->AudioMaster());
			convertersContainer->AddUG(converter);
			converter->SetupDynamicPlugs();
			converter->connect( from_plug, converter->plugs[converterInputPlug] );
			converter->SetUnusedPlugs2();
		}

		from_plug = converter->plugs[converterOutputPlug]; // will be connected below..
	}

	// do we need to insert an adder?
	if( to_plug->InUse() && to_plug->DataType != DT_MIDI2 ) // midi don't need adder
	{
		assert( to_plug->DataType == DT_FSAMPLE );
		// adders must NEVER be inserted once sound starts, even to cope with new voice
		// they won't have correct sort order, they won't be opened correctly (because may be added to wrong voice)
		// ALL adders MUST be inserted during setup
#ifdef DEBUG_CONECTIONS
		if( SampleClock() != 0 )
		{
			_RPT0(_CRT_WARN, "Can't insert adders after start clock.  Should already be done!!!\n");
		}

#endif

		if( to_plug->IsAdder() )
		{
			ug_adder2* a2 = dynamic_cast<ug_adder2*>(to_plug->UG );
			assert( a2 && "only adder2 supported here at present");
			a2->NewConnection(from_plug);
		}
		else // need a new adder
		{
			// input already in use
			// find 'old' input and connector
			UPlug* old_from = to_plug->connections.front();

			// now insert an adder
			ug_base* adder = to_plug->UG->parent_container->InsertAdder( old_from->DataType );
			ug_adder2* a2 = dynamic_cast<ug_adder2*>(adder);
			assert( a2 && "only adder2 supported here at present");
			if (a2) // patch cables from snapshots can accidentally try to connect to DT_INTs toggether, which is not supported.
			{
				a2->patch_control_container = to_plug->UG->patch_control_container;

				to_plug->DeleteConnection(old_from);

				// connect adder input to 'from_plug'
#ifdef DEBUG_CONECTIONS
				_RPT2(_CRT_WARN, "{ Connecting %s plug %d to adder\n", from_plug->UG->DebugModuleName(), from_plug->getPlugIndex());
#endif
				a2->NewConnection(from_plug);// , adder->plugs[1]);
#ifdef DEBUG_CONECTIONS
				_RPT2(_CRT_WARN, "{ Connecting %s plug %d to adder\n", old_from->UG->DebugModuleName(), old_from->getPlugIndex());
#endif
				a2->NewConnection(old_from); //adder->plugs[1]);
				// change from_plug to adder's output, carry on.
				// connect adder's output to original to-plug
				adder->connect(adder->plugs[0], to_plug);
				// ensure future connects are diverted to adder
				to_plug->Proxy = adder->plugs[1];
#ifdef DEBUG_CONECTIONS
				_RPT2(_CRT_WARN, "{ Connecting adder to %s plug %d\n", to_plug->UG->DebugModuleName(), to_plug->getPlugIndex());
#endif
			}
		}

		return;
	}

	// Set input variable pointer to address of an output variable
	void** input, *output;
	input = (void**) to_plug->io_variable;
	output = from_plug->io_variable;
	from_plug->connections.push_back(to_plug);
	to_plug->connections.push_back(from_plug);

	{
		if( from_plug->DataType == DT_MIDI2 && output != NULL )
		{
			((midi_output*) output)->SetUp(from_plug);
		}

		if( input != NULL && output != NULL )
		{
			switch(from_plug->DataType)
			{
				// This is required. Link sample buffer to destination.
			case DT_FSAMPLE:
					*input = output;
				break;

			case DT_MIDI2:  //  not required. MIDI is event-driven.
					break;

			case DT_BLOB:	// new datatype in 1.1. Don't need backward compatibility.
			case DT_BLOB2:
				break;

				// copy current output value to destination plug.
				// From old-days when first-sample status-send was not madatory.
				// !!! SHOULD NOT BE REQUIRED ANYMORE !!! REMOVE IN 2.0
			case DT_ENUM:
					*((short*)to_plug->io_variable) = *((short*)output);
				break;

			case DT_INT:
					*((int*)to_plug->io_variable) = *((int*)output);
				break;

			case DT_BOOL:
					*((bool*)to_plug->io_variable) = *((bool*)output);
				break;

			case DT_FLOAT:
					*((float*)to_plug->io_variable) = *((float*)output);
				break;

			case DT_DOUBLE:
					*((double*)to_plug->io_variable) = *((double*)output);
				break;

			case DT_TEXT:
					*((std::wstring*)to_plug->io_variable) = *((std::wstring*)output);
				break;

			case DT_STRING_UTF8:
					*((std::string*)to_plug->io_variable) = *((std::string*)output);
				break;

			default:
				{
				    assert(false);
				}
				};
		}
	}
}

void ug_base::SendPendingOutputChanges()
{
	for( auto p : plugs)
	{
		if( (p->flags & PF_VALUE_CHANGED) != 0 && p->Direction == DR_OUT )
		{
			assert( p->DataType != DT_MIDI2 );

			if( p->DataType == DT_FSAMPLE )
			{
				OutputChange( SampleClock(), p, ST_ONE_OFF );
			}
			else
			{
				SendPinsCurrentValue( SampleClock(), p );
			}

			p->ClearFlag( PF_VALUE_CHANGED );
		}
	}
}

#if 0
void ug_base::OnUiNotify2( int p_msg_id, my_input_stream& p_stream )
{
	switch( p_msg_id )
	{
	case code_to_long('s','e','t','d'): // "setd" - Set Plug Default. Editor only.
		{
		    std::wstring defaultValue;
		    int pinIdx;
		    p_stream >> pinIdx;
		    p_stream >> defaultValue;

			// When 
			if (pinIdx < plugs.size())
			{
				GetPlug(pinIdx)->SetDefault2(WStringToUtf8(defaultValue).c_str());
			}
		}
		break;
	}
}
#endif

void ug_base::AttachDebugger()
{
	ug_base* clone = this;

	// When debugging container inside oversampler
	// measure CPU on oversampler (to include oversampler's overhead).
	if( ParentContainer() == 0 )
	{
		ug_oversampler* oversampler = dynamic_cast<ug_oversampler*>( clone->AudioMaster() );

		if( oversampler != 0 )
		{
			clone = oversampler;
		}
	}

	while( clone )
	{
		// kick start measurer (todo , stop it)
		assert(!clone->m_debugger);
		clone->m_debugger = std::make_unique<UgDebugInfo>(clone);

		clone = clone->m_next_clone;
	}
}

void ug_base::AddPlug(UPlug* plug)
{
	// TEST try skipping this (id should never be confused with index)
	// plugs without specific Unique-ID are assigned one based on position.
// !!! hmm, just mimiking index. yet some unit tests fail without it.??????
	if( plug->UniqueId() == -1 )
	{
		plug->setUniqueId( (int) plugs.size() );
	}

	plug->setIndex( (int) plugs.size() );
	plugs.push_back( plug );
}

void activateFeedbackModule(ug_base* feedback_out)
{
	// User has inserted a feedback module, activate it by removing dummy 'fuse' connection.
	auto dummy = feedback_out->plugs[0];
	dummy->connections.front()->connections.clear();
	dummy->connections.clear();
}

FeedbackTrace* ug_base::CalcSortOrder3(int& maxSortOrderGlobal)
{
	if (SortOrder2 >= 0) // Skip sorted modules.
		return {};

	SortOrder2 = -2; // prevent recursion back to this.

	for (auto p : plugs)
	{
		if (p->Direction != DR_IN)
			continue;

		for (auto from : p->connections)
		{
			const int order = from->UG->SortOrder2;
			if (order == -2) // Found an feedback path, report it.
			{
				// feedback-out encounters feedback on it's own helper.
				if (
					moduleType && (moduleType->GetFlags() & CF_IS_FEEDBACK) != 0
					&& p->UniqueId() == 0
					&& from->UG->moduleType && (from->UG->moduleType->GetFlags() & CF_IS_FEEDBACK) != 0 // ensure this is the 'OUT' module
					)
				{
					activateFeedbackModule(this);
					// we have invalidated the iterator over 'p->connections'.
					// need to exit the loop. note that with feedback_delay_out, there are no other input plugs anyhow, so it is safe to skip the remaining pins.
					goto done;
				}
				else
				{
					SortOrder2 = -1; // Allow this to be re-sorted after feedback (potentially) compensated.
					auto e = new FeedbackTrace(SE_FEEDBACK_PATH);
					e->AddLine(p, from);
					return e;
				}
			}

			if (order == -1) // Found an unsorted path, go up it.
			{
				auto e = from->UG->CalcSortOrder3(maxSortOrderGlobal);

				if (e) // Upstream module encountered feedback.
				{
					// Not all modules have valid moduleType, e.g. oversampler_in
					if (
						moduleType && (moduleType->GetFlags() & CF_IS_FEEDBACK) != 0
						&& p->UniqueId() == 0
						&& from->UG->moduleType && (from->UG->moduleType->GetFlags() & CF_IS_FEEDBACK) != 0 // ensure this is the 'OUT' module
						)
					{
						activateFeedbackModule(this);

						// Feedback fixed, remove feedback trace.
						delete e;
						e = nullptr;

						// Continue as if nothing happened.
						goto done;
					}
					else
					{
						SortOrder2 = -1; // Allow this to be re-sorted after feedback (potentially) compensated.

						// If downstream module has feedback, add trace information.
						e->AddLine(p, from);
						if (e->feedbackConnectors.front().second.moduleHandle == Handle()) // only reconstruct feedback loop as far as nesc.
						{
#if defined( _DEBUG )
							e->DebugDump();
#endif
							throw e;
						}
						return e;
					}
				}
			}
		}
	}

done:

	SortOrder2 = ++maxSortOrderGlobal;

#if 0 // _DEBUG
	_RPTN(_CRT_WARN, " SortOrder: %3d  ", SortOrder2);
	DebugIdentify();
	_RPT0(_CRT_WARN, "\n");
#endif

	return nullptr;
}

void ug_base::OnUiMsg(int p_msg_id, my_input_stream& p_stream)
{
	// wake it
	if( sleeping && (flags & UGF_SUSPENDED) == 0 )
	{
		Wake();
	}

	OnUiNotify2( p_msg_id, p_stream );
}

// display a message in user interface thread
void ug_base::message(std::wstring msg, int type)
{
	// can't access user interface from background thread
	// so send a message
	msg = DebugModuleName() + L" : " + msg;

	my_msg_que_output_stream strm( AudioMaster()->getShell()->MessageQueToGui(), UniqueSnowflake::APPLICATION, "mbox");
	strm << (int) ( sizeof(wchar_t) * msg.size() + sizeof(int) + sizeof(type)); // message length.
	strm << msg;
	strm << type;

	strm.Send();
}

// *** shared Interpolation filter setup ***
const float* ug_base::GetInterpolationtable()
{
	const int table_width = INTERPOLATION_POINTS / 2;
	const int table_entries = ( table_width + 1 ) * INTERPOLATION_DIV;
	float* interpolation_table2;
	CreateSharedLookup2( (L"Sine-C Interpolation"), m_shared_interpolation_table, -1, table_entries * 2, true, SLS_ALL_MODULES  );

	//if( !m_shared_interpolation_table->GetInitialised() )
	//{
	//	m_shared_interpolation_table->SetSize(table_entries * 2); //604);
	//}

	interpolation_table2 = (float*) m_shared_interpolation_table->table; // + table_entries; //302;

	if( !m_shared_interpolation_table->GetInitialised() )
	{
		// create a pointer to array with 'negative' indices
		// initialise interpolation table
		// as per page 523 MAMP
		// create vitual array with -ve indices
		//	_RPT0(_CRT_WARN,"\n");
		// check not initialised already, entry 11 should be 0.801....
		assert( fabs(interpolation_table2[11] - .801) > .1 ); // negative index!!!|| interpolation_table2[11] != interpolation_table2[-11] );
		int i;

		//		_RPT0(_CRT_WARN," IDX   WSINC  = SINC  *  HANNING\n");
		for( int sub_table = 0 ; sub_table < INTERPOLATION_DIV ; sub_table++ )
		{
			int table_index = sub_table * INTERPOLATION_POINTS + INTERPOLATION_POINTS/2 - 1;

			for( int x = -table_width ; x < table_width; x++ )
			{
				i = sub_table + x * INTERPOLATION_DIV;
				// position on x axis
				double o = (double) i / INTERPOLATION_DIV;
				// filter impulse response
				double sinc = sin( PI * o ) / ( PI * o );
				// apply tailing function
				double hanning = cos( 0.5 * PI * i / ( INTERPOLATION_DIV * table_width ) );
				float windowed_sinc = (float)(sinc * hanning * hanning);
				//				_RPT4(_CRT_WARN,"%3d %+.5f   %+.5f %+.5f\n", i, windowed_sinc, sinc, hanning );
				assert( table_index >= 0 && table_index < table_entries * 2);
				interpolation_table2[table_index-x] = windowed_sinc;
				//				interpolation_table2[table_index-x] = windowed_sinc;
			}

			//			_RPT0(_CRT_WARN,"\n");
		}

		interpolation_table2[table_width-1] = 1.f; // fix div by 0 bug
		// first table copied to last, shifted 1 place
		int idx = INTERPOLATION_DIV * INTERPOLATION_POINTS;

		for( int table_entry = 1 ; table_entry <= INTERPOLATION_POINTS ; table_entry++ )
		{
			interpolation_table2[idx + table_entry] = interpolation_table2[table_entry-1];
		}

		//		interpolation_table2[idx+INTERPOLATION_POINTS-1] = 0.f;
		interpolation_table2[idx] = 0.f;

		/* print out interpolation table
				for( int sub_table = 0 ; sub_table <= INTERPOLATION_DIV ; sub_table++ )
				{
					for( int x = 0 ; x < INTERPOLATION_POINTS; x++ )
					{
						int table_index = sub_table * INTERPOLATION_POINTS + x;
						int linear_index = sub_table + INTERPOLATION_DIV * x;
						_RPT2(_CRT_WARN,"%d %+.5f\n",linear_index, interpolation_table2[table_index] );
					}
				}
		*/
		// additional fine tuning.  Normalise all 32 posible filters so total gain is always 1.0
		// This fixes 'overtones' problems, due to different sub-filters having slightly different overall gains
		for( int sub_table = 0 ; sub_table <= INTERPOLATION_DIV ; sub_table++ )
		{
			int table_index = sub_table * INTERPOLATION_POINTS;
			assert( table_index >= 0 && table_index < table_entries * 2);
			// calc sub table sum
			double fir_sum = 0.f;

			for( int table_entry = 0 ; table_entry < INTERPOLATION_POINTS ; table_entry++ )
			{
				fir_sum += interpolation_table2[table_index+table_entry];
			}

			// use it to normalise sub table
			for( int table_entry = 0 ; table_entry < INTERPOLATION_POINTS ; table_entry++ )
			{
				float adjusted = interpolation_table2[table_index+table_entry] / (float)fir_sum;

				if (fpclassify(adjusted) == FP_SUBNORMAL)
					adjusted = 0.f;

				interpolation_table2[table_index+table_entry] = adjusted;
			}
		}

		m_shared_interpolation_table->SetInitialised();
	}

	return interpolation_table2;
}

void ug_base::HandleEvent(SynthEditEvent* e)
{
	//	assert( !sleeping );
	assert( e->timeStamp == SampleClock() );

	switch( e->eventType )
	{
		case UET_EVENT_STREAMING_START:
		case UET_EVENT_STREAMING_STOP:
		case UET_EVENT_SETPIN: //UET _STAT_CHANGE:
			{
				UPlug* to_plug = GetPlug( e->parm1 );
				state_type new_state = ST_ONE_OFF;

				if( e->eventType == UET_EVENT_STREAMING_START )
				{
					new_state = ST_RUN;
				}

				assert( to_plug->DataType == DT_FSAMPLE || new_state != ST_RUN ); // ST_RUN only supported from streaming samples
				to_plug->setState( new_state );

				onSetPin( e->timeStamp, to_plug , new_state );

				if (new_state == ST_ONE_OFF) // one-offs need re-set once processed
				{
					to_plug->setState(ST_STOP);  //hmmm!!!
				}
			}
			break;

		case UET_RUN_FUNCTION2:
		{
			ug_func func[2] = {nullptr, nullptr}; // prevent stack overwrite on 32-bit systems.
		    // only copies some of function pointer, assumes remainder is zeroed.
		    int32_t* i = (int32_t*) &func;
		    i[0] = e->parm2;
		    i[1] = e->parm3;
		    (this->*(func[0]))();
		}
		break;

		case UET_VOICE_STREAMING_STATE:
		{
			const auto voiceNumber = e->parm3;
			ParentContainer()->at(voiceNumber)->outputSilentSince = (e->parm1 == ST_RUN) ? std::numeric_limits<timestamp_t>::max() : e->timeStamp;
		}
		break;
		
		case UET_VOICE_LEVEL:
		{
			const auto& voiceNumber = e->parm3;
			const auto peakOutputLevel = *((float*)&(e->parm2)); // nasty.

			auto voice = ParentContainer()->at(voiceNumber);
			voice->peakOutputLevel = peakOutputLevel;

			// this is a backup for special cases like 'VoiceMonitorFallback' test where UET_VOICE_STREAMING_STATE never comes.
			if (e->parm1 != ST_RUN && voice->outputSilentSince == std::numeric_limits<timestamp_t>::max())
			{
				voice->outputSilentSince = e->timeStamp;
			}

			// check for voice's that missed suspension by UET_VOICE_DONE_CHECK because they were still held.
			voice->DoneCheck(e->timeStamp);
		}
		break;
		
		case UET_VOICE_DONE_CHECK:
		{
			// voice has been silent for a whole block, check if it's OK to suspend it.
			const auto& voiceNumber = e->parm3;
			auto voice = ParentContainer()->at(voiceNumber);
			voice->DoneCheck(e->timeStamp);
		}
		break;
#if 0	
	// natural note end causes voice-active signal to go low.
	case UET_DEACTIVATE_VOICE:
		{
			Voice* voice = ParentContainer()->at( e->parm3 );
//			_RPT3(_CRT_WARN, "ug(%x) UET_DEACTIVATE_VOICE voice %d at time %d\n", this, voice->m_voice_number, e->timeStamp );

		    //_RPT1(_CRT_WARN, "block-pos %d\n", e->timeStamp % AudioMaster()->BlockSize() );
		    // Check new note hasn't soft-stolen voice since it went quiet.
		    if( !voice->isHeld() )
			{
				//_RPT1(_CRT_WARN, "TS %d\n", e->timeStamp );
				//_RPT1(_CRT_WARN, "V%d voiceState_ = VS_MUTING (UET_DEACTIVATE_VOICE)\n", voice->m_voice_number );

				const float voiceActive = 0.0f;
				ParentContainer()->SetVoiceParameters( e->timeStamp, voice, voiceActive );

				// Prevent voice being allocated until properly shut down.
				voice->voiceState_ = VS_MUTING;
			}
			else
			{
				//_RPT3(_CRT_WARN, "ug(%x) UET_DEACTIVATE_VOICE voice %d at time %d - IGNORED (voice playing)\n", this, voice->m_voice_number, e->timeStamp );
			}
		}
		break;
#endif
		
	case UET_SUSPEND:
		ParentContainer()->suspendVoice(e->timeStamp, e->parm3);
		break;

	case UET_GRAPH_START:
	case UET_EVENT_MIDI:
		break;

	default:
		assert( false ); // un-handled event
	};
}

bool ug_base::isEventListEmpty()
{
	return events.empty();
}

// send ug to sleep at first opertunity,
// won't actually sleep untill no events remain
void ug_base::process_sleep(int /*start_pos*/, int /*sampleframes*/)
{
#ifdef _DEBUG
	VerifyOutputsConsistant();
#endif

	SleepMode();
}

// do nothing, handy when just woken, but stat changes havn't arrived, yet, prevent's it going right back to sleep
void ug_base::process_nothing(int /*start_pos*/, int /*sampleframes*/)
{
}

void ug_base::VerifyOutputsConsistant()
{
#if 0 // TODO !!!! def _DEBUG

	for( vector<UPlug*>::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
	{
		UPlug* plg = *it;

		if( plg->DataType == DT_FSAMPLE && plg->Direction == DR_OUT )
		{
			if( (flags & UGF_DONT_CHECK_BUFFERS ) == 0 && !plg->GetSampleBlock()->CheckAllSame() ) // trying to suspend when outputs not 'flat'
			{
				DebugTimestamp();
				DebugIdentify();
				_RPT0(_CRT_WARN, "!!! VerifyOutputsConsistant() FAIL !!!!\n" );
				//				assert(false);
			}
		}
	}

#endif
}

// this is similar to suspend, but 'wakes' the ug if any event, stat changes etc arrive
void ug_base::SleepMode()
{
	if( events.empty() )	// can't sleep if more events due
	{
		if( !sleeping )
		{
#ifdef _DEBUG
			VerifyOutputsConsistant();
#endif
			AudioMaster()->SuspendModule(this);
			sleeping = true;
		}
	}
}

void ug_base::ResetStaticOutput()
{
	static_output_count = AudioMaster()->BlockSize();
}

void ug_base::QueProgramChange( timestamp_t /*p_clock*/, int /*p_patch_num*/ )
{
	// SDK2 only now. see ug_plugin.
}

void ug_base::SetPolyphonic(bool p)
{
	SetFlag( UGF_POLYPHONIC, p );

#ifdef _DEBUG
	if (SeAudioMaster::GetDebugFlag(DBF_TRACE_POLY))
	{
		DebugPrintName(true);
		_RPT0(_CRT_WARN, "- SetPolyphonic\n");
	}
#endif
}

void ug_base::SetPPDownstreamFlag(bool p)
{
	SetFlag( UGF_POLYPHONIC_DOWNSTREAM, p );
}

ug_base* ug_base::CloneOf()
{
	return m_clone_of;
}

// Equivalent without using CUG.
void ug_base::SetUnusedPlugs2()
{
	for( auto p : plugs )
	{
		if( p->Direction == DR_IN && !p->InUse() && p->DataType != DT_MIDI && p->Proxy == 0 ) // MIDI-CV redirects MIDI-Chan. Don't set it (crash).
		{
			std::string defaultVal;
			if( moduleType ) // oversamplers have null moduletype. But don't have default values anyhow.
			{
				auto desc = moduleType->getPinDescriptionById(p->UniqueId()); // Wave Recorder 2 connected to a muted module will get nullptr here.
				if (desc)
				{
					defaultVal = WStringToUtf8(desc->GetDefault(0));
				}
			}
			p->SetDefault(defaultVal.c_str());
		}
	}
}

void ug_base::SumCpu(float cpu_block_rate)
{
	if (cpuCycleTotal == 0.0f && cpuPeakCycles == 0.0f) // attempt to save cache thrashing on parent object members.
	{
		return;
	}

	// Parent containers record sum of all modules within.
	if (cpuParent)
	{
		cpuParent->cpuCycleTotal += cpuCycleTotal;
	}

	const float averageCpu = cpuCycleTotal / cpu_block_rate;

	// If debugger window open, pass it the cpu too. (via the main clone).
	if (m_debugger)
	{
		CloneOf()->m_debugger->AddCpu(cpuCycleTotal, cpuPeakCycles);
	}

	// Clear out total ready for next round.
	cpuPeakCycles = cpuCycleTotal = 0.0f;
}

void ug_base::ClearBypassRoutes(int32_t inPlugIdx, int32_t outPlugIdx)
{
	UPlug* ip = GetPlug(inPlugIdx);
	assert(ip->Direction == DR_IN);

    if( ip->Proxy == 0 ) // already not bypassed?
        return;

	UPlug* op = GetPlug(outPlugIdx);
	assert(op->Direction == DR_OUT);

	// clear bypass pointers.
	ip->Proxy = 0;
	op->Proxy = 0;

	AddBypassRoutePt2(op, op);
}

bool ug_base::AddBypassRoute(int32_t inPlugIdx, int32_t outPlugIdx)
{
	UPlug* ip = GetPlug(inPlugIdx);
	assert(ip->Direction == DR_IN);
	UPlug* op = GetPlug(outPlugIdx);
	assert(op->Direction == DR_OUT);

    if( ip->Proxy == op ) // already bypassed?
        return true;

	// Record bypass.
	ip->Proxy = op;
	op->Proxy = ip;

	UPlug* bypassSourcePlug = ip->connections[0];
	while( bypassSourcePlug->Proxy != 0 )
	{
		assert(bypassSourcePlug->Proxy->Direction == DR_IN);
		bypassSourcePlug = bypassSourcePlug->Proxy->connections[0];
	}

	bool canSleep = AddBypassRoutePt2(bypassSourcePlug, op);

#ifdef _DEBUG
	if( canSleep ) // fill original output buffer with loud noise.
	{
		float* block_ptr = op->GetSamplePtr();
		float s = 30000;
		int blocksize = AudioMaster()->BlockSize();
		for( int i = 0; i < blocksize; ++i )
		{
			*block_ptr++ = s;
			s = -s;
		}
	}
#endif

	return canSleep;
}

bool ug_base::AddBypassRoutePt2(UPlug* bypassSourcePlug, UPlug* outPlug)
{
	bool canSleep = true;
	for( vector<UPlug*>::iterator it = outPlug->connections.begin(); it != outPlug->connections.end(); ++it )
	{
		UPlug* to = *it;
	
		canSleep &= to->UG->BypassPin(bypassSourcePlug, to);
		if( to->Proxy != 0 )
		{
			assert(to->Proxy->Direction == DR_OUT);
			// This module is already bypassed, look ahead.
			canSleep &= AddBypassRoutePt2(bypassSourcePlug, to->Proxy);
		}
	}

	return canSleep;
}

bool ug_base::BypassPin(UPlug* fromPin, UPlug* toPin)
{
	assert(toPin->Direction == DR_IN);

	toPin->io_variable = &( fromPin->io_variable );
	if( toPin->sample_ptr )
	{
		*( toPin->sample_ptr ) = toPin->GetSamplePtr();
	}
	return true;
}

int ug_base::calcDelayCompensation()
{
	if (cumulativeLatencySamples != LATENCY_NOT_SET)
	{
		return cumulativeLatencySamples;
	}

	#if defined( DEBUG_LATENCY )
	std::stringstream log;
	#endif

	// scan upstream modules to get their latency.
	cumulativeLatencySamples = 0;
	int minUpstreamLatency = (numeric_limits<int>::max)();
	for (auto p : plugs)
	{
		if (p->Direction == DR_IN)
		{
			for (auto fromPlug : p->connections)
			{
				int upstreamLatency = fromPlug->UG->calcDelayCompensation();
				cumulativeLatencySamples = (std::max)(cumulativeLatencySamples, upstreamLatency);
				minUpstreamLatency = (std::min)(minUpstreamLatency, upstreamLatency);
			}
		}
	}

	// Is there any latency mismatch?
	if (minUpstreamLatency != cumulativeLatencySamples && minUpstreamLatency != (numeric_limits<int>::max)())
	{
		// insert latency compensation.
		for (auto p : plugs)
		{
			if (p->Direction == DR_IN)
			{
				auto safeCopyOfConnections = p->connections;
				for (auto fromPlug : safeCopyOfConnections)
				{
					int upstreamLatency = fromPlug->UG->calcDelayCompensation();
					if (upstreamLatency < cumulativeLatencySamples)
					{
						const auto compensationSamples = cumulativeLatencySamples - upstreamLatency;
						#if defined( DEBUG_LATENCY )
							//_RPT1(_CRT_WARN, " Inserted %d compensation from ", compensationSamples);
							//fromPlug->UG->DebugIdentify();
							//_RPT0(_CRT_WARN, "\n");
							log << " Inserted " << compensationSamples << " compensation from " << WStringToUtf8(fromPlug->UG->DebugModuleName()) << "\n";
						#endif

						// now insert an latency adjustor
						const wchar_t* adjustor_type = nullptr;

						if(p->DataType == DT_FSAMPLE)
						{
							adjustor_type = L"SE LatencyAdjust";
						}
						else
						{
							adjustor_type = L"SE LatencyAdjust-eventbased2";
						}

						// find 'old' input and connector
						fromPlug->DeleteConnection(p);

						auto compensatorType = ModuleFactory()->GetById(adjustor_type);
						auto compensator = compensatorType->BuildSynthOb();
						AudioMaster()->AssignTemporaryHandle(compensator);
						compensator->AddFixedPlugs();
						compensator->SetAudioMaster(AudioMaster());
						parent_container->AddUG(compensator);
						compensator->SetupDynamicPlugs();

						if (p->DataType == DT_FSAMPLE)
						{
							auto compensatorActual = dynamic_cast<LatencyAdjust*>(dynamic_cast<ug_plugin3Base*>(compensator)->GetGmpiPlugin());
							compensatorActual->latency = compensationSamples;
						}
						else
						{
							auto compensatorActual = dynamic_cast<LatencyAdjustEventBased2*>(compensator);
							compensatorActual->datatype = p->DataType;
							//compensatorActual->latencySamples = compensationSamples;

							// Add two plugs to compensator of the relevent datatype
							compensator->AddPlug(new UPlug(compensator, DR_IN, p->DataType));
							compensator->AddPlug(new UPlug(compensator, DR_OUT, p->DataType));
						}

						// connect compensator input to 'from_plug'
						compensator->connect(fromPlug, compensator->plugs[0]);
						auto saveProxy = p->Proxy;
						p->Proxy = nullptr; // else connection will go elsewhere.
						compensator->connect(compensator->plugs[1], p);
						p->Proxy = saveProxy; // restore.

						// Poly Patch parameters come from pins marked "PF_POLYPHONIC_SOURCE", but PPS is NOT itself polyphonic.
						if ((fromPlug->UG->GetPolyphonic() || fromPlug->GetFlag(PF_POLYPHONIC_SOURCE)) && (GetPolyphonic() || GetFlag(UGF_POLYPHONIC_AGREGATOR) ))
						{
							compensator->SetPolyphonic(true);
						}

						compensator->cumulativeLatencySamples = cumulativeLatencySamples; // prevent calc being performed on adjustor.
						compensator->latencySamples = compensationSamples; // latency on compensator.
						// CAUSES EXTRA ADDERS TO BE INSERTED WHEN p is voice adder!!!!
						//p->Proxy = compensator->plugs[0]; // helps pin default changes go to default_setter and not compensator output pin.
					}
				}
			}
		}
	}

	if (latencySamples != 0) // avoid virtual function call if not needed.
	{
		// Add my own latency
		cumulativeLatencySamples += latencySamples;

		// Constraint
		cumulativeLatencySamples = (std::min)(cumulativeLatencySamples, AudioMaster()->latencyCompensationMax() );
	}

	#if defined( DEBUG_LATENCY )
		DebugIdentify();
		_RPT2(_CRT_WARN, ". Latency %d, cumulative %d\n", latencySamples, cumulativeLatencySamples);
		const auto s = log.str();
		if(!s.empty())
		{
			_RPT1(_CRT_WARN, "%s\n", s.c_str());
		}
	#endif

	return cumulativeLatencySamples;
}

// Hack! Only for modules requires special privileges from audiomaster. iterates up though oversamplers to true audiomaster.
SeAudioMaster* ug_base::AudioMaster2()
{
	auto am = AudioMaster();
	while (dynamic_cast<ug_oversampler*>(am) != nullptr)
	{
		am = dynamic_cast<ug_oversampler*>(am)->AudioMaster();
	}
	assert(dynamic_cast<SeAudioMaster*>(am));

	return dynamic_cast<SeAudioMaster*>(am);
}

void CreateMidiRedirector(ug_base* midiCv)
{
	if (midiCv->GetPlug(0)->DataType == DT_MIDI)
	{
		auto redirector = ModuleFactory()->GetById(L"SE MIDICVRedirect")->BuildSynthOb();

		midiCv->parent_container->AddUG(redirector);
		redirector->patch_control_container = midiCv->patch_control_container;
		/*
		Problem, in 'UnterminatedPoly_Patch_point" unit test, this causes the MIDI player2 to be delayed by one block.
		*/
		redirector->SetupWithoutCug();
		redirector->SetFlag(UGF_MIDI_REDIRECTOR); // prevent feedback error when controls connected to modules upstream of MIDI-CV MIDI port.

		// old way was to connect dummy to MIDI-CV. Which only works if there is one MIDI-CV only and no Keyboard2.

		// Because MIDI Redirect sends it's Voice controls via Patch-parameter-setter,
		// it needs to always be upstream of it. Else events can arrive late to secondary modules (e.g. keyboard2).
		auto redirectorDummyOut = redirector->GetPlug(2);
		//shitty, design flaw		midiCv->connect(redirectorDummyOut, midiCv->ParentContainer()->GetParameterSetter()->GetPlug(0));
		// reinstated for depth-first sort. Else MCV Redirect can end up downstream of stuff it's controlling indirectly VIA dsp-patch-manager -> PPS.
		// ref: oversampled_synth_no_patch_mgr.se1
		midiCv->connect(redirectorDummyOut, midiCv->ParentContainer()->GetParameterSetter()->GetPlug(0));

		midiCv->connect(redirectorDummyOut, midiCv->GetPlug(0)); // -> MIDI-CV.MIDI-In

		// Redirect any MIDI connections to redirector.
		midiCv->GetPlug(0)->Proxy = redirector->GetPlug(0); // Redirector MIDI In.
		midiCv->GetPlug(1)->Proxy = redirector->GetPlug(1); // MIDI Chan.
		redirector->cpuParent = midiCv->cpuParent;
	}
}
