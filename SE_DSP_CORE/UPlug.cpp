#include "pch.h"

#include "UPlug.h"
#include "ug_container.h"
#include "./modules/se_sdk3/mp_sdk_common.h"
#include "module_info.h"
#include "ug_patch_param_setter.h"
#include "UMidiBuffer2.h"
#include "./modules/shared/RawView.h"
#include "SeAudioMaster.h"
#include "BundleInfo.h"

using namespace std;


UPlug::UPlug(ug_base* ug, InterfaceObject& pdesc) :
	Direction( pdesc.GetDirection() )
	,Proxy(NULL)
	,state(ST_STOP)
	,DataType( pdesc.GetDatatype() )
	,UG( ug )
	,TiedTo(NULL)
	,io_variable( pdesc.GetVarAddr() )
	,flags(PF_PP_NONE)
	,sample_ptr(pdesc.sample_ptr)
	,m_last_one_off_time(SE_TIMESTAMP_MAX)
	,uniqueId_( pdesc.getPlugDescID(0) )
#if defined( _DEBUG )
	,debug_sent_status(false)
#endif
{
	// no, is used. assert( (int)sample_ptr == 0 ); // testin to see if it's ever used, if not remove InterfaceObject.sample_ptr
	assert( sample_ptr != (float**) 0x01010101 );

	if( pdesc.GetPPActiveFlag() )
	{
		SetFlag( PF_PP_ACTIVE );
	}

	if( ( pdesc.GetFlags() & IO_ADDER ) != 0)
	{
		SetFlag( PF_ADDER );
	}

	// tREAT TEXT LIKE blob, NO LOCALE OR CODE-PAGE INFLUENCE.
	if( ( pdesc.GetFlags() & IO_BINARY_DATA ) != 0)
	{
		SetFlag( PF_BINARY_DATA );
	}

	if( ( pdesc.GetFlags() & (IO_UI_DUAL_FLAG|IO_PATCH_STORE) ) != 0)
	{
		// fix for DH Volt to float, which incorrectly sets a volt-in to patch store (DR_IN, patch store can't have regular connections)
		//		if( pdesc.GetDirection() == DR_OUT || ( pdesc.GetFlags() & IO_PRIVATE ) != 0 )
		//		{
		SetFlag( PF_PATCH_STORE );
		//		}
		//		else
		//		{
		//			_RPT0(_CRT_WARN, "WARNING, module attempting to set non-private input plug as PATCH STORE\n" );
		//		}
	}

	if( ( pdesc.GetFlags() & IO_HOST_CONTROL ) != 0 )
	{
		SetFlag( PF_HOST_CONTROL );
	}

	// IO_UI_DUAL_FLAG is obsolete, any plug using it needs direction reversed
	if( ( pdesc.GetFlags() & IO_UI_DUAL_FLAG ) != 0)
	{
		if( Direction == DR_OUT )
		{
			Direction = DR_IN;
		}
		else
		{
			Direction = DR_OUT;
		}
	}
}

// This version used for container IO fake plugs and parameters and .sep plugs
UPlug::UPlug( ug_base* ug, EDirection dr, EPlugDataType dt ) :
	Direction( dr )
	,Proxy(NULL)
	,state(ST_STOP)
	,DataType(dt)
	,UG(ug)
	,TiedTo(NULL)
	,io_variable(NULL)
	,flags(PF_PP_NONE)
	,sample_ptr(NULL)
	,m_last_one_off_time(SE_TIMESTAMP_MAX)
	,uniqueId_(-1)
#if defined( _DEBUG )
	,debug_sent_status(false)
#endif
{
	/* does not appear to be the case anymore. also causes problem for .sep with MIDI outs (prevents automatic creation of midi_out variable)
	if you uncomment this, also uncomment stuff in ug_plugin::Setup DynamicPlugs()

		// SPECIAL CASE
		// unused midi inputs are not connected anywhere (unused audio inputs are connected to io mod)
		// so if a MIDI input is connected to an IO MOD, BUT there is nothing connected
		// to the outside container plug, CRASH!, io_variable pointer remains NULL
		// this fixes it
		if( DataType == DT_MIDI2 && Direction == DR_OUT )
		{
			io_variable = &ug_base::default _midibuffer;
		}
	*/
}

// used during clone of a ug, SHOULD ONLY BE USED FOR DYNAMIC PLUGS
UPlug::UPlug( ug_base* ug, UPlug* copy_plug ) :
	Direction( copy_plug->Direction )
	,Proxy(NULL)
	,state(ST_STOP)
	,DataType(copy_plug->DataType)
	,UG( ug )
	,TiedTo(NULL)
	,io_variable(NULL)
	,flags( copy_plug->flags )
	,sample_ptr(NULL)
	,m_last_one_off_time(SE_TIMESTAMP_MAX)
	,uniqueId_( copy_plug->UniqueId() )
#if defined( _DEBUG )
	,debug_sent_status(false)
#endif
{
}

UPlug::~UPlug()
{
	// you must delete connectors b4 plug
	// else strange feedback occurs.
	DestroyBuffer();
}

void UPlug::DestroyBuffer()
{
	// delete attached buffers
	if( (flags & PF_OWNS_BUFFER) != 0 )
	{
		switch( DataType )
		{
		case DT_MIDI2:
		{
			if( Direction == DR_IN )
			{
				//					delete m_buffer.midi_in;
			}
			else
			{
				delete m_buffer.midi_out;
			}
		}
		break;

		case DT_TEXT:
		{
			delete m_buffer.text_ptr;
		}
		break;

		case DT_STRING_UTF8:
		{
			delete m_buffer.text_utf8_ptr;
		}
		break;

		case DT_BLOB:
		{
			delete (MpBlob*) m_buffer.generic_pointer;
		}
		break;

		case DT_FSAMPLE:
		{
			if( Direction == DR_OUT )
			{
				assert(BundleInfo::instance()->isEditor); // only meant to be editor when user changes pin value

				delete [] m_buffer.buffer;
				m_buffer.buffer = {};
			}
			else
			{
				io_variable = 0;
			}
		}
		break;

		case DT_DOUBLE:
		{
			delete m_buffer.double_ptr;
		}
		break;

		case DT_ENUM:
		case DT_FLOAT:
		case DT_BOOL:
		case DT_INT:
		{
			assert(false); // should not be needed
			// nothing to do
		}
		break;

		default:
			assert(false);
		};
	}

	ClearFlag(PF_OWNS_BUFFER);
}

void UPlug::DeleteConnectors()
{
	// Delete all connectors (co-operates with DisConnect() )
	for( vector<UPlug*>::iterator it = connections.begin(); it != connections.end(); ++it )
	{
		( *it )->DisConnect(this);
	}
	connections.clear();
}

//!!! plugs dctr delete conectors, conectors try to talk to plugs!!! crash!!!
void UPlug::DeleteConnection(UPlug* other)
{
	DisConnect(other);
	other->DisConnect(this);
}

void UPlug::DisConnect(UPlug* from)
{
	auto it = find(connections.begin(), connections.end(), from);

	if( it != connections.end() )
	{
		connections.erase(it);
	}
}

UPlug* UPlug::GetTiedTo()
{
	return TiedTo;
}

// Polyphonic determination helper routines
struct FeedbackTrace* UPlug::PPSetDownstream()
{
	SetFlag( PF_PP_DOWNSTREAM );
	// notify 'to' UG
	return UG->PPSetDownstream();
}
/* shitty idea
// pp-set-downstream may originate outside of oversampler in a higher container,
// in which case "tunnel" down to poly container before setting downstream flags.
struct FeedbackTrace* UPlug::PPSetDownstreamTunneling(ug_container* voiceControlContainer)
{
	for (auto p : connections)
	{
		auto fbTrace = p->UG->PPSetDownstreamTunneling(p, voiceControlContainer);

		if (fbTrace)
			return fbTrace;
	}
/ *
	auto oversampler = dynamic_cast<ug_oversampler*>(UG);
	if (oversampler)
	{
		oversampler->insidePlugs[uniqueId_]->PPSetDownstreamTunneling(voiceControlContainer);
	}
	else
	{
		if (UG->ParentContainer()->getOutermostPolyContainer() == voiceControlContainer)
		{
			return PPSetDownstream();
		}
		else
		{
			for (auto p : connections)
			{
				auto fbTrace = p->PPSetDownstreamTunneling(voiceControlContainer);

				if (fbTrace)
					return fbTrace;
				/ *
							auto ioPlug = p->GetTiedTo(); // inside container
							if (ioPlug)
							{
								for (auto otherPlug : ioPlug->connections)
								{
									auto fbTrace = otherPlug->PPSetDownstreamTunneling(voiceControlContainer);

									if (fbTrace)
										return fbTrace;
								}
							}
							* /
			}
		}
	}
	* /
	return nullptr;
}
*/

// indicate if USER has connected something to this plug
// (ignoring unused inputs with a default connection)
bool UPlug::HasNonDefaultConnection()
{
	if( connections.empty() )
		return false;

	if( Direction == DR_IN ) // inputs fed by a default value don't count as 'in-use'
	{
		UPlug* from = connections.front();
		// No longer using I/O Mod. New XML method creates all modules BEFORE setting defaults,
		// therefore it's possible for an I/O mod to exist with output connections...
		// using THAT as a default setter results in feedback loop. (previous, a 'blank I.O mod got created by first call here).
		// IO Mod is used as source of un-connected values
		// ug_default_setter is used as source of un-connected values
		return ( from->UG->GetFlags() & UGF_DEFAULT_SETTER ) == 0;
	}

	return true;
}

// container and IO plugs may need to be diverted to
// their real destination
void UPlug::ReRoute()
{
	UPlug* tiedto = GetTiedTo();

	// always an IO Mod output...
	while( !connections.empty() )
	{
		UPlug* tp = connections.back();
		connections.pop_back();
		DeleteConnection(tp);

		// for each wire to proxy, make new connection.
		for( vector<UPlug*>::iterator it = tiedto->connections.begin(); it != tiedto->connections.end(); ++it )
		{
			UPlug* otherPlug = *it;
			// can't connect a polyphonic module in one container to a polyphonic in annother
			// the two voices are not related.
			// was happening with polyphonic MIDI from CV-MIDI to MIDI-CV.
			assert( !otherPlug->UG->GetPolyphonic() || !tp->UG->GetPolyphonic() );
			UG->connect( otherPlug, tp );

			// In editor, when user types new default value into Container input, need to forward the SetDefault to the correct destination.
			if( Direction == DR_OUT )
			{
				tiedto->Proxy = tp;
			}
		}
	}

	// once done, delete all prox's connections..
/*
	while( !tiedto->connectors.empty() )
	{
		delete tiedto->connectors.front();
	}

	tiedto->connectors.clear();
*/
	tiedto->DeleteConnectors();
}

void UPlug::debug_dump()
{
	std::wstring dtype = L"unknown";
	std::wstring dir = L"unknown";

	switch( Direction )
	{
	case DR_IN:
		dir = L"DR_IN";
		break;

	case DR_OUT:
		dir = L"DR_OUT";
		break;
            
    default:
        break;
	};

//	_RPTW1(_CRT_WARN, L"  %s \n", dir );
}

bool UPlug::IsAdder()
{
	return ( flags & PF_ADDER ) != 0;//DataType == DT_SAMP_ARRAY || DataType == DT_MIDI_ARRAY;
}

#if 0
// return pointer to array of samples
USampBlock* UPlug::GetSampleBlock()
{
	assert( DataType == DT_FSAMPLE );
	assert( io_variable != NULL );

	if( Direction == DR_OUT )
	{
		return (USampBlock*) io_variable;
	}

	assert( Direction == DR_IN );
	return *(USampBlock**) io_variable;
}
#endif

// !!! OLD: ASSUMES PLUGS HOLD POINTER TO IO VAR (NO LONGER TRU SDK3) !!!
// !!! USE  ug_base::TransmitPinValue() instead
// sends state changes to downstream ugs.
// stores current state to avoid duplicate messages
// however ST_ONE_OFF is stored as ST_STOP to allow subsequent msgs to be sent
void UPlug::TransmitState(timestamp_t p_clock, state_type p_stat)
{
#if defined( _DEBUG )
	debug_sent_status = true;
	/*
			// best to send events due within current block.
			if( p_clock >= UG->AudioMaster()->BlockStartClock() && p_clock < UG->AudioMaster()->NextGlobalStartClock() )
			{
				_RPT0(_CRT_WARN, "Event timestamp outside current block!!!\n" );
			}
	*/
#endif
	assert( DataType == DT_FSAMPLE ); // only streaming pins supported here. Use ug_base::SendPinsCurrentValue(UPlug *)

	// Avoid unnesc ST_STOP, ST_RUN msgs
	if( p_stat == state && p_stat != ST_ONE_OFF )
	{
		return;
	}

	// VCS5 seems to propagate reams of stat change messages,
	// Avoid unnesc ST_ONE_OFF on audio outputs
	if( m_last_one_off_time == p_clock && p_stat == ST_ONE_OFF && state < ST_RUN ) // avoid duplicate one-off changes
	{
		return;
	}

	state = p_stat;

	assert( p_clock >= UG->SampleClock() );
	assert( Direction == DR_OUT );

	for( auto to : connections )
	{
		auto toUg = to->UG;
		assert(toUg->SortOrder2 > UG->SortOrder2);

		if( toUg->IsSuspended() )
		{
			assert(to->DataType != DT_MIDI);
			toUg->DiscardOldPinEvents(to->getPlugIndex());
		}

		ug_event_type eventType;

		if( p_stat == ST_RUN )
			eventType = UET_EVENT_STREAMING_START;
		else
			eventType = UET_EVENT_STREAMING_STOP;

		toUg->AddEvent(
#if defined( SE_FIXED_POOL_MEMORY_ALLOCATION )
		    toUg->AudioMaster()->AllocateMessage(
#else
		    new_SynthEditEvent(
#endif
				p_clock,
		        eventType,
				to->getPlugIndex(),	// int 1	- pin Index
		        0,					// int 2	- data size in bytes
		        0,					// parm3	- data (if size <= 4 bytes)
		        0					// parm4
		        )					// extra	- data (if size > 4 bytes)
		);
	}

	if( state == ST_ONE_OFF) // allow subsequent one_offs to register
	{
		m_last_one_off_time = p_clock;
		state = ST_STOP;
	}
	else
	{
		m_last_one_off_time = 0;
	}
}

// return current input value for float plugs
// relies on sample clock member up to date
float UPlug::getValue()
{
	assert( UG->sleeping == false );

	assert( UG->getBlockPosition() >= 0 && UG->getBlockPosition() < (timestamp_t) UG->AudioMaster()->BlockSize() );
	return * ( GetSamplePtr() + UG->getBlockPosition() );
}

// Set default immediate. Assumes caller in DSP thread. Only used in SE Editor.
void UPlug::SetDefault2(const char* utf8val)
{
	assert( DataType != DT_MIDI2 && (flags & PF_HOST_CONTROL) == 0 );

	if( Direction == DR_OUT )
	{
		UG->Wake(); // This is called in response to an event on the DESTINATION plug. This module likely alseep.
		SetBufferValue(utf8val);

//		assert(connections.size() == 1);
		if( DataType == DT_FSAMPLE )
		{
			UG->OutputChange( UG->SampleClock(), this, ST_ONE_OFF );
		}
		else
		{
			UG->SendPinsCurrentValue( UG->SampleClock(), this );
		}
	}
	else // find default-setter --feeding--> this
	{
		// User changed default on Container input, forward it.
		if( Proxy )
		{
			assert(connections.empty() || Proxy->UG->getModuleType()->UniqueId() == L"SE LatencyAdjust-eventbased2" || Proxy->UG->getModuleType()->UniqueId() == L"SE LatencyAdjust"); // must be container input, should be re-routed
			Proxy->SetDefault2(utf8val);
			return;
		}

		assert(connections.size() == 1 && (Direction == DR_IN));
		auto from = connections.front();

		// Might be Latency-adjuster between module and default-setter.
		// If so, search backward for default-setter.
		while (!from->UG->GetFlag(UGF_DEFAULT_SETTER))
		{
			from = from->UG->plugs[0]->connections.front();
			assert(from);
		}

		assert(from->UG->SortOrder2 < UG->SortOrder2); // default-setter must be upstream.

		auto fromUg = from->UG;
		auto fromIdx = from->getPlugIndex();
		while (fromUg)
		{
			fromUg->plugs[fromIdx]->SetDefault2(utf8val);
			fromUg = fromUg->m_next_clone;
		}
	}
}

// unconnected plugs are connected to an default-setter, which creates the output buffer.
// the default-setter's output is then set to the plug's default value.
void UPlug::SetDefault(const char* utf8val)
{
	if( DataType == DT_MIDI2 )
	{
		return;
	}

	if( (flags & PF_HOST_CONTROL) != 0 )
	{
		return;
	}

	if( Direction == DR_OUT )
	{
		if (DataType == DT_FSAMPLE)
		{
			// SetupDynamicPlugs() will have already registered the pin as a regular output.
			// but it's most likely a 'Fixed Values'. Unregister it first..
			UG->AudioMaster2()->UnRegisterPin(this);
			UG->AudioMaster2()->RegisterConstantPin(this, utf8val);
		}
		else
		{
			SetBufferValue(utf8val);
		}

		SetFlag( PF_VALUE_CHANGED );
		// All real-time updates to a module's default should go via message que and SetDefault2()
		// This function should be purely for initial setup.
		assert(( UG->flags & UGF_OPEN ) == 0 );
	}
	else // input OR parameter needs update
	{
		//* OLD - buggy with un-connected container inputs connected on inside to ENUM.
		if( TiedTo && TiedTo->Proxy )
		{
			assert(connections.empty()); // must be container input, should be re-routed
			TiedTo->Proxy->SetDefault(utf8val);
			return;
		}

		if (!connections.empty()) // already connected to io-mod. Forward to it.
		{
			if (Proxy) // special case for Poly-to_Mono, when input not connected.
			{
				assert(UG->getModuleType()->UniqueId() == L"SE Poly to Mono");

				Proxy->SetDefault(utf8val);
				return;
			}

			assert(connections.size() == 1 && (Direction == DR_IN /*|| Direction == DR _PARAMETER*/));
			UPlug* from = connections.front();
			assert(from->UG->SortOrder2 < UG->SortOrder2); // io_mod must be upstream

			from->SetDefault(utf8val);
		}
		else // then need to add one
		{
			assert(connections.empty() && (Direction == DR_IN));
			assert((UG->flags & UGF_OPEN) == 0); // DEFAULT VALUE SHOULD ALREADY EXIST IF SYNTH RUNNING
			// add new parameter to container's io/mod
			// connect this to the specified input
			// ( to ensure uparameters are in same container as ug)
			ug_base* pset;

			int newPlugsFlags = 0;
			if ((flags & PF_PATCH_STORE) != 0)
			{
				pset = UG->parent_container->GetParameterSetter();
				newPlugsFlags = PF_PATCH_STORE;
			}
			else
			{
				// note: HasNonDefaultConnection() relys on this
				pset = UG->parent_container->GetDefaultSetter();
			}

			UPlug* p = new UPlug(pset, DR_OUT, DataType);
//				p->CreateBuffer();
			p->SetFlag(newPlugsFlags);
			pset->AddPlug(p);
			pset->connect(p, this);// Connect plug to new parameter.

			// Don't set flag on ParameterSetter, causes pin's blank initial value to be sent, overriding event sent earlier by
			// dsp_patch_parameter_base::OnValueChanged
			if ((flags & PF_PATCH_STORE) == 0)
			{
				if (p->DataType == DT_FSAMPLE)
				{
					UG->AudioMaster2()->RegisterConstantPin(p, utf8val);
				}
				else
				{
					p->CreateBuffer();
					p->SetBufferValue(utf8val);
				}

				p->SetFlag(PF_VALUE_CHANGED);
			}
			else
			{
				p->CreateBuffer();
			}

			// Container inputs will later be re-routed directly to destination.
			// however user might still set default on the container input (while engine running).
			// we need to use Proxy pointer to point to correct IOMod plug.
			if (TiedTo)
			{
				assert(TiedTo->Proxy == 0);
				TiedTo->Proxy = p;
			}
		}
	}
}

// Send an event to pins as if it was connected to a default setter (but don't bother to actually connect it).
// !! not working polyphonic !! perhaps need to clone events on unconnected inputs too
void UPlug::SetDefaultDirect(const char* utf8val)
{
	RawData raw;
	char* temp;

	switch (DataType)
	{
	case DT_FLOAT:
	{
		auto v = (float)strtod(utf8val, &temp);
		raw = RawView(v);
	}
	break;

	case DT_DOUBLE:
	{
		auto v = (double)strtod(utf8val, &temp);
		raw = RawView(v);
	}
	break;

	case DT_FSAMPLE:
	{
		assert(false); // not handled
	}
	break;

	case DT_ENUM:
	{
		auto v = (short)strtol(utf8val, &temp, 10);
		raw = RawView(v);
	}
	break;

	case DT_INT:
	{
		auto v = (int32_t)strtol(utf8val, &temp, 10);
		raw = RawView(v);
	}
	break;

	case DT_TEXT:
	{
		auto v = Utf8ToWstring(utf8val);
		raw = RawView(v);
	}
	break;
	
	case DT_STRING_UTF8:
	{
		const std::string v(utf8val);
		raw = RawView(v);
	}
	break;

	case DT_BOOL:
	{
		auto v = (bool)strtol(utf8val, &temp, 10) != 0;
		raw = RawView(v);
	}
	break;

	case DT_BLOB: // not handled, perhaps could support hex !!!
		break;

	default:
		assert(false); // you are using an undefined datatype
	}

	UG->SetPinValue(UG->AudioMaster()->NextGlobalStartClock(), getPlugIndex(), DataType, raw.data(), static_cast<int32_t>(raw.size()));
}

void UPlug::AssignBuffer(float* buffer)
{
	m_buffer.buffer = buffer;

	for (auto from : connections)
	{
		from->m_buffer.buffer = buffer;
	}
}

// All float outs have buffer created for them.  ug_plugin uses this for the other datatypes when module doesn't provide them (usually on dynamic plugs)
void UPlug::CreateBuffer()
{
	// All samples outputs need a buffer created
	// if ug provides float* in io_variable, it can be overwitten as it's copied to UPlug.sample_ptr
	if( DataType == DT_FSAMPLE) // should really be handled by sub-classing
	{
		if( Direction == DR_OUT )
		{
			UG->AudioMaster2()->RegisterPin(this);
		}
		else
		{
			io_variable = &m_buffer.buffer;
		}
		return;
	}

	// for all other types, only need to setup io_variable if plugin didn't provide pointer to variable. (only SDK modules at present use this)
	if( io_variable == 0 )
	{
		switch( DataType ) // should really be handled by sub-classing
		{
		case DT_MIDI2:
		{
			SetFlag( PF_OWNS_BUFFER );

			if( Direction != DR_IN )
			{
				assert( m_buffer.midi_out == 0 ); // ALREADY created (memory leak).
				m_buffer.midi_out = new midi_output();
				io_variable = m_buffer.midi_out;
			}
		}
		break;

		case DT_TEXT:
		{
			assert( m_buffer.text_ptr == 0 ); // ALREADY created (memory leak).

			SetFlag( PF_OWNS_BUFFER );
			m_buffer.text_ptr = new std::wstring();
			io_variable = m_buffer.text_ptr;
		}
		break;

		case DT_STRING_UTF8:
		{
			assert( m_buffer.text_ptr == 0 ); // ALREADY created (memory leak).

			SetFlag( PF_OWNS_BUFFER );
			m_buffer.text_utf8_ptr = new std::string();
			io_variable = m_buffer.text_utf8_ptr;
		}
		break;

		case DT_BLOB:
		{
			assert( m_buffer.generic_pointer == 0 ); // ALREADY created (memory leak).

			SetFlag( PF_OWNS_BUFFER );
			m_buffer.generic_pointer = new MpBlob;
			io_variable = m_buffer.generic_pointer;
		}
		break;

		case DT_DOUBLE:
		{
			assert( m_buffer.double_ptr == 0 ); // ALREADY created (memory leak).

			SetFlag( PF_OWNS_BUFFER );
			m_buffer.double_ptr = new double;
			io_variable = m_buffer.double_ptr;
		}
		break;

		case DT_ENUM:
		case DT_FLOAT:
		case DT_BOOL:
		case DT_INT:
		{
			// if plugin didn't provide storage for the variable, provide it ourself
			if( io_variable == 0 )
			{
				io_variable = &m_buffer.float_ob;
			}
		}
		break;

		default:
			assert(false); // unsupported type (add it here + destructor)
		};
	}
}

void UPlug::SetBufferValue( const char* p_val )
{
	char* temp;

	switch( DataType )
	{
	case DT_FLOAT:
	{
		//float* pd = (float*) io_variable;
		//*pd = (float) StringToDouble(p_val);
		*((float*) io_variable) = (float) strtod( p_val, &temp );
	}
	break;

	case DT_DOUBLE:
	{
		*((double*) io_variable) = strtod( p_val, &temp );
	}
	break;

	case DT_FSAMPLE:
	{
		assert(BundleInfo::instance()->isEditor); // only used in editor

		const float fs = (float) strtod( p_val, &temp ) / MAX_VOLTS;
		const auto bs = UG->AudioMaster()->BlockSize();

		if (!GetFlag(PF_OWNS_BUFFER))
		{
			m_buffer.buffer = new float[bs];
			if (sample_ptr)
			{
				*(sample_ptr) = m_buffer.buffer;
			}

			SetFlag(PF_OWNS_BUFFER);

			for (auto c : connections)
			{
				c->m_buffer.buffer = m_buffer.buffer;
				if (c->sample_ptr) // internal modules.
				{
					*(c->sample_ptr) = m_buffer.buffer;
				}

				c->UG->OnBufferReassigned();
			}
		}

		for (int i = 0; i < bs; ++i)
			m_buffer.buffer[i] = fs;
	}
	break;

	case DT_ENUM:
	{
		//*((short*) io_variable) = StringToInt(p_val);
		*((short*) io_variable) = (short) strtol( p_val, &temp, 10 );
	}
	break;

	case DT_INT:
	{
		//*((int*) io_variable) = StringToInt(p_val);
		*((int*) io_variable) = strtol( p_val, &temp, 10 );
	}
	break;

	case DT_TEXT:
		*(std::wstring*)io_variable = Utf8ToWstring(p_val);
		break;

	case DT_STRING_UTF8:
		*(std::string*)io_variable = p_val;
		break;

	case DT_BOOL:
	{
		*((bool*) io_variable) = strtol( p_val, &temp, 10 ) != 0;
	}
	break;

	case DT_BLOB: // not handled, perhaps could support hex !!!
		break;

	default:
		assert(false); // you are using an undefined datatype
	}
}

void UPlug::CloneBufferValue( const UPlug& clonePlug )
{
	switch( DataType )
	{
	case DT_FLOAT:
	{
		*( (float*) io_variable ) = *( (float*) clonePlug.io_variable );
	}
	break;

	case DT_DOUBLE:
	{
		*( (double*) io_variable ) = *( (double*) clonePlug.io_variable );
	}
	break;

	case DT_FSAMPLE:
	{
		m_buffer.buffer = clonePlug.m_buffer.buffer; // use same fixed value
/* should not be nesc?
		float fs = ((USampBlock*) clonePlug.io_variable)->GetAt(0);
		((USampBlock*) io_variable)->SetAll( fs, UG->AudioMaster()->BlockSize() );
*/
	}
	break;

	case DT_ENUM:
	{
		*( (short*) io_variable ) = *( (short*) clonePlug.io_variable );
	}
	break;

	case DT_INT:
	{
		*( (int*) io_variable ) = *( (int*) clonePlug.io_variable );
	}
	break;

	case DT_TEXT:
		*( (std::wstring*) io_variable ) = *( (std::wstring*) clonePlug.io_variable );
		break;

	case DT_STRING_UTF8:
		*( (std::string*) io_variable ) = *( (std::string*) clonePlug.io_variable );
		break;

	case DT_BOOL:
	{
		*( (bool*) io_variable ) = *( (bool*) clonePlug.io_variable );
	}
	break;

	case DT_BLOB: // not handles, perhaps could support hex !!!
		break;

	default:
		assert(false); // you are using an undefined datatype
	}
}

// replaces UPlug::TransmitState()
void UPlug::Transmit(timestamp_t timestamp, int32_t data_size, const void* data)
{
	assert( Direction == DR_OUT );

	#if defined( _DEBUG )
		if( timestamp == 0 )
			debug_sent_status = true;
	#endif

	for( auto& to_plug : connections )
	{
		to_plug->UG->SetPinValue(timestamp, to_plug->getPlugIndex(), DataType, data, data_size);
	}
}

void UPlug::TransmitPolyphonic(timestamp_t timestamp, int physicalVoiceNumber, int32_t data_size, const void* data)
{
	assert( Direction == DR_OUT );

	#if defined( _DEBUG )
		if( timestamp == 0 )
			debug_sent_status = true;
	#endif

	// All voices are connected to this one pin, need to send value only to correct voice.
	for( auto it = connections.begin(); it != connections.end(); ++it )
	{
		UPlug* to_plug = *it;
		ug_base* ug = to_plug->UG;

		if( ug->pp_voice_num == physicalVoiceNumber && ug->GetPolyphonic() ) // Don't set voice-active on non-polyphonic modules in Voice 0. That pin should remain at default value.
		{
			ug->SetPinValue(timestamp, to_plug->getPlugIndex(), DataType, data, data_size);
		}
	}
}

bool UPlug::CheckAllSame()
{
	const float* buf = m_buffer.buffer;
	const auto blockSize = UG->AudioMaster()->BlockSize();

	for (int i = 0; i < blockSize; ++i)
	{
		if (buf[0] != buf[i])
			return false;
	}
	
	return true;
}
