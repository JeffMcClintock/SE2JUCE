#include "pch.h"
#include <algorithm>
// Fix for <sstream> on Mac (sstream uses undefined int_64t)
#include "./modules/se_sdk3/mp_api.h"
#include <sstream>
#include "ug_oversampler.h"
#include "USampBlock.h"
#include "ug_container.h"
#include "ug_oversampler_in.h"
#include "ug_oversampler_out.h"

#include "EventProcessor.h"
#include "DspPatchManagerProxy.h"
#include "./modules/shared/xplatform.h"

using namespace std;

#define MAX_DEBUG_BUFFERS 40

ug_oversampler::ug_oversampler() :
	oversampleFactor_(0)
	, filterType_(0)
	,clientSampleClock(0)
#if defined( _DEBUG )
	// old way. kept in debug for verification new way works.
,clientClockAtMyBlockStart(0)
#endif
	,clientBlockStartClock(0)
	,clientNextGlobalStartClock_(0)
	,clientClockOffset_(0)
	,moduleInsertClock_(0)
	,dspPatchManager(0)
{
}

void ug_oversampler::Setup1(int factor, int filterType, bool copyStandardPlugs )
{
	// Poles: [1-9] IIR filter, [10-20] FIR filter quality setting, [20-] FIR Filter literal number of taps.
	filterType_ = filterType;
	oversampleFactor_ = factor;

// WRONG	main_container->patch_control_container = main_container;

	main_container->SetAudioMaster(this);

	oversampler_out = new ug_oversampler_out();
	AudioMaster()->AssignTemporaryHandle(oversampler_out);
	oversampler_out->oversampler_ = this;
	oversampler_out->oversampleFactor_ = oversampleFactor_;
	oversampler_out->SetAudioMaster(this);
	main_container->AddUG( oversampler_out );

	oversampler_in = new ug_oversampler_in();
	AudioMaster()->AssignTemporaryHandle(oversampler_in);
	oversampler_in->oversampler_ = this;
	oversampler_in->oversampleFactor_ = oversampleFactor_;
	oversampler_in->SetAudioMaster(this);
	main_container->AddUG( oversampler_in );

	// input latency of oversampler.
	oversampler_in->calcLatency(oversampleFactor_);
	oversampler_out->calcLatency(filterType, oversampleFactor_);

	{
		// copy any dynamic plugs. (SDK3 relies on this to add all plugs)
		assert( plugs.size() == 0 );

		// In SE, oversamplers have copies of all the containers pins, in XML builds the first 3 are excluded.
		int copyFrom = CONTAINER_FIXED_PLUG_COUNT;
		if( copyStandardPlugs )
		{
			copyFrom = 0;
			for( int x = 0 ; x < CONTAINER_FIXED_PLUG_COUNT ; ++x )
			{
				InsidePlugs.push_back( 0 );
			}
		}

		// no need to copy first 3 property pins (they arn't exposed for connections).
		for( unsigned int x = copyFrom; x < main_container->plugs.size() ; x++ )
		{
			UPlug* p1 = main_container->plugs[x];
			UPlug* p2 = new UPlug( this, p1 );
			AddPlug( p2 );
			p1->Proxy = p2; // ensure connections go to OVERSAMPLER.

			if( x >= CONTAINER_FIXED_PLUG_COUNT )
			{
				UPlug* insidePlug;
				if( p1->Direction == DR_OUT )
				{
					insidePlug = oversampler_out->AddProxyPlug(p2);
				}
				else
				{
					// most of the time, a polyphonic signal going into an oversampler should make it polyphonic.
					// This flag ensures that
					p2->SetFlag(PF_PP_ACTIVE);

					insidePlug = oversampler_in->AddProxyPlug(p2);
				}

				InsidePlugs.push_back( insidePlug );
			}
		}
	}

	// create plug buffers.
	SetupDynamicPlugs();
}

// After cloned for oversampling.
void ug_oversampler::Setup1b( int factor, int filterType, bool copyStandardPlugs )
{
	filterType_ = filterType;
	oversampleFactor_ = factor;

//	SetCUG(main_container->Get CUG());
	main_container->patch_control_container = main_container;

	oversampler_out->oversampler_ = this;
	oversampler_out->oversampleFactor_ = oversampleFactor_;
	assert(oversampler_out->AudioMaster() == this);

	oversampler_in->oversampler_ = this;
	oversampler_in->oversampleFactor_ = oversampleFactor_;
	assert(oversampler_in->AudioMaster() == this);

	{
		// then copy any dynamic plugs. (SDK3 relies on this to add all plugs)
		// In SE, oversamplers have copies of all the containers pins, in XML builds the first 3 are excluded.
		int copyFrom = CONTAINER_FIXED_PLUG_COUNT;
		/*
		// didn't work when patch-cables add extra plugs into oversampler.
		bool copyStandardPlugsTest = m_clone_of->plugs.size() == main_container->plugs.size();
		*/
		// new: Detect non-XML build by detecting first 3 inside plugs pointers set to null.
		auto prototype = dynamic_cast<ug_oversampler*>(m_clone_of);
		bool copyStandardPlugsTest = prototype->InsidePlugs.size() >= CONTAINER_FIXED_PLUG_COUNT && prototype->InsidePlugs[0] == nullptr;

		if (copyStandardPlugsTest)
		{
			copyFrom = 0;
			for( int x = 0 ; x < CONTAINER_FIXED_PLUG_COUNT ; ++x )
			{
				InsidePlugs.push_back( 0 );
			}
		}
		// no need to copy first 3 property pins (they arn't exposed for connections).
		int outIdx = 0;
		int inIdx = 0;
		int myIdx = 0;
		for( unsigned int x = copyFrom; x < main_container->plugs.size() ; x++ )
		{
			UPlug* p1 = main_container->plugs[x];
			UPlug* p2 = plugs[myIdx++];
			p1->Proxy = p2; // ensure connections go to OVERSAMPLER.

			if( x >= CONTAINER_FIXED_PLUG_COUNT )
			{
				UPlug* insidePlug;
				if( p1->Direction == DR_OUT )
				{
					insidePlug = oversampler_out->GetProxyPlug( outIdx++, p2 );
				}
				else
				{
					insidePlug = oversampler_in->GetProxyPlug( inIdx++, p2 );
				}

				InsidePlugs.push_back( insidePlug );
			}
		}

		// setup additional host-controls pins, that were added later by SE.
		for (size_t x = InsidePlugs.size(); x < plugs.size(); ++x)
		{
			UPlug* p2 = plugs[myIdx++];

			UPlug* insidePlug;
			if (p2->Direction == DR_OUT)
			{
				insidePlug = oversampler_out->GetProxyPlug(outIdx++, p2);
			}
			else
			{
				insidePlug = oversampler_in->GetProxyPlug(inIdx++, p2);
			}

			InsidePlugs.push_back(insidePlug);
		}
	}

	//	SetupDynamicPlugs();
}

UPlug* ug_oversampler::routePatchCableOut(UPlug* plug)
{
	auto outsidePlug = new UPlug(this, plug);
	AddPlug(outsidePlug);
	if (outsidePlug->DataType == DT_FSAMPLE)
	{
		outsidePlug->CreateBuffer();
	}
	//p1->Proxy = p2; // ensure connections go to OVERSAMPLER.

	UPlug* insidePlug;
	if (plug->Direction == DR_OUT)
	{
		insidePlug = oversampler_out->AddProxyPlug(outsidePlug);
	}
	else
	{
		insidePlug = oversampler_in->AddProxyPlug(outsidePlug);
	}
	InsidePlugs.push_back(insidePlug);

	if (plug->Direction == DR_OUT)
	{
		connect(plug, insidePlug);
	}
	else
	{
		connect(insidePlug, plug);
	}

	return outsidePlug;
}

void ug_oversampler::CreatePatchManagerProxy()
{
	if( main_container->has_own_patch_manager() == false )
	{
		dspPatchManager = new DspPatchManagerProxy( patch_control_container, this );
		main_container->setPatchManager( dspPatchManager );
	}
}

void ug_oversampler::Setup2(bool xmlMethod)
{
	/*
	// Print out plugs
	_RPTW0(_CRT_WARN, L"\n-Inner Container----------------\n" );
	for( auto it = main_container->plugs.begin() ; it != main_container->plugs.end() ; ++ it )
	{
		_RPTW3(_CRT_WARN, L"%d %s %s\n", (*it)->getPlugIndex(), directionToString((*it)->Direction).c_str(), datatypeToString((*it)->DataType).c_str() );
	}

	_RPTW0(_CRT_WARN, L"\n-oversampler_out----------------\n" );
	for( auto it = oversampler_out->plugs.begin() ; it != oversampler_out->plugs.end() ; ++ it )
	{
		_RPTW3(_CRT_WARN, L"%d %s %s\n", (*it)->getPlugIndex(), directionToString((*it)->Direction).c_str(), datatypeToString((*it)->DataType).c_str() );
	}

	_RPTW0(_CRT_WARN, L"\n-oversampler_in----------------\n" );
	for( auto it = oversampler_in->plugs.begin() ; it != oversampler_in->plugs.end() ; ++ it )
	{
		_RPTW3(_CRT_WARN, L"%d %s %s\n", (*it)->getPlugIndex(), directionToString((*it)->Direction).c_str(), datatypeToString((*it)->DataType).c_str() );
	}
*/
	// Re-route plugs via oversampler_in and oversampler_out modules.
	int index_in = 0;
	int index_out = 0;
	for( unsigned int x = CONTAINER_FIXED_PLUG_COUNT; x < main_container->plugs.size() ; ++x )
	{
		UPlug* p = main_container->plugs[x];

		if( p->Direction == DR_OUT )
		{
			UPlug* op = oversampler_out->plugs[index_out++];
			// BEFORE.
			// [source inside cont]-[IOMod.Input]-[container.output]-[destination outside]
			// AFTER.
			// [source]-[OS Out]-[Oversampler.output]-[destination]
			// Connect [source]-[OS Out].
			if( p->GetTiedTo()->InUse() )
			{
				UPlug* tiedTo = p->TiedTo;
				while( !tiedTo->connections.empty() )
				{
					UPlug* from = tiedTo->connections.front();
					oversampler_out->connect(from, op);
					// delete old connection.
					from->DeleteConnection(tiedTo);
				}

				// Delete old IO Mod plug (else SE tries to set it's 'default' and crashes).
				tiedTo->UG->plugs.erase(find(tiedTo->UG->plugs.begin(), tiedTo->UG->plugs.end(), tiedTo) );
				delete tiedTo;
				p->TiedTo = 0;
			}
			else
			{
				// Line connected on outside only.
				op->SetDefault("");
			}

			// assume outputs have only open connction, and we have re-routed it.
			assert(p->connections.empty());
		}
		else // DR_IN
		{
			// BEFORE.
			// [source outside cont]-[container.input]-[IOMod.output]-[destination inside cont]
			// AFTER.
			// [source]-[Oversampler.input]-[OS In]->[destination]
			// Connect [OS In]->[destination].
			// no. would insert adder - oversampler_in->connect( op, c->To );
#if 0
			// the oversampler_in pin that we are going to move connections on the inside to.
			UPlug* op = oversampler_in->plugs[index_in++];

			auto old_inward_io_mod_pin = p->GetTiedTo();
			while( !old_inward_io_mod_pin->connections.empty() )
			{
				// remove connection from io-mod, note it's destination
				auto to_plug = old_inward_io_mod_pin->connections.back();
				old_inward_io_mod_pin->connections.pop_back();

				// add connection from oversampler-in to destination
				auto from_plug = op;
				assert( from_plug->DataType == to_plug->DataType );
				from_plug->connections.push_back(to_plug);
				to_plug->connections.push_back(from_plug);

				// copy over buffer pointers
				if( from_plug->DataType == DT_FSAMPLE )
				{
					void** input = (void**) to_plug->io_variable;
					void* output = from_plug->io_variable;
					*input = output;
				}

				// delete old connection to the inner module
				to_plug->DisConnect(old_inward_io_mod_pin);
			}
#endif
			//////////////////////////
			
			// the oversampler_in pin that we are going to move connections on the inside to.
			auto from_plug = oversampler_in->plugs[index_in++];

			auto inside_pin = p->GetTiedTo();
			for (auto to_plug : inside_pin->connections)
			{
				// add connection from oversampler-in to destination
				from_plug->connections.push_back(to_plug);
				to_plug->connections.push_back(from_plug);

				assert(from_plug->DataType == to_plug->DataType);

				// copy over buffer pointers
				if (from_plug->DataType == DT_FSAMPLE)
				{
					void** input = (void**)to_plug->io_variable;
					void* output = from_plug->io_variable;
					*input = output;
				}

				// delete old connection to the inner module
				to_plug->DisConnect(inside_pin);

			}
			inside_pin->connections.clear();

			//////////////////////////
						
			// route container default-setter connections to oversampler.
			// (no other wires are hooked up yet)
			for (auto outerConnection : p->connections)
			{
				// connect default_setter to oversampler
				connect(outerConnection, plugs[x - CONTAINER_FIXED_PLUG_COUNT]);

				// delete old connection from the default-setter to the container
				outerConnection->DisConnect(p);
			}
			p->connections.clear();
		}
	}

	main_container->PostBuildStuff(xmlMethod);
}

ug_base* ug_oversampler::Clone( CUGLookupList& UGLookupList )
{
	// Moved so new container can be passed correct audiomaster
	auto oversampler = (ug_oversampler*)Create();
	SetupClone(oversampler);

	CUGLookupList templist;
	// Important that child container has correct audiomaster, not same as the container it's copying.
	auto childContainer = dynamic_cast<ug_container*>( main_container->Copy(oversampler, templist) );
	oversampler->main_container = childContainer;
	
	// let it know what container will send it program changes (it's 'original' container)
	assert( childContainer->patch_control_container == main_container->patch_control_container );
	assert(oversampler->AudioMaster() == AudioMaster());

	oversampler->oversampler_out = dynamic_cast<ug_oversampler_out*>(templist.Lookup2(oversampler_out));
	oversampler->oversampler_in = dynamic_cast<ug_oversampler_in*>(templist.Lookup2(oversampler_in));
	oversampler->Setup1b( oversampleFactor_, filterType_);
	oversampler->CreatePatchManagerProxy(); // here to prevent crash in Voice::PolyphonicSetup()

	oversampler->main_container->PostBuildStuff(false);

	return oversampler;
}

UPlug* ug_oversampler::CreateProxyPlug( UPlug* destination )
{
	// Oversampler's proxyplug on outside.
	UPlug* p2 = new UPlug( this, destination );
	p2->CreateBuffer();
	AddPlug( p2 );

	UPlug* insidePlug;
	if( destination->Direction == DR_OUT )
	{
		insidePlug = oversampler_out->AddProxyPlug(p2);
		InsidePlugs.push_back( insidePlug );
		connect( destination, insidePlug );
	}
	else
	{
		insidePlug = oversampler_in->AddProxyPlug(p2);
		InsidePlugs.push_back( insidePlug );
		connect( insidePlug, destination );
	}

	return p2;
}

int ug_oversampler::Open()
{
	// Setup CPU Pollong.
	AudioMaster()->AddCpuParent(this);

	main_container->Open(); // Open all unit generators

	activeModules.SortAll(nonExecutingModules);

	main_container->cpuParent = 0; // prevent CPU counting twice.
	// CPU reporting to pass total to CContainer. So borrow it's handle. NON STANDARD.
	SetHandle( main_container->Handle() );

	// needs to be zero while openeing children, then initiialised properly.
	clientNextGlobalStartClock_ = BlockSize();
#ifdef _DEBUG
	dbg_copy_output_array.assign(MAX_DEBUG_BUFFERS,(USampBlock*)0);

	for( int i = 0 ; i < MAX_DEBUG_BUFFERS ; ++i )
	{
		dbg_copy_output_array[i] = new USampBlock( BlockSize() );
	}

#endif
	return ug_base::Open();
}

int ug_oversampler::Close()
{
	main_container->CloseAll();
	return ug_base::Close();
}

void ug_oversampler::Wake()
{
	// un tested.
	ug_base::Wake();

	clientClockOffset_ = clientSampleClock - ug_base::SampleClock() * oversampleFactor_;
}

void ug_oversampler::Resume()
{
	ug_base::Resume();
	//_RPT0(_CRT_WARN, "Resume\n" );

	// Block positions get screwed up on resume when buffer_offset non-zero (host splits blocks). Re-sync them.

	// Correct clientClockAtMyBlockStart.
	int buffer_offset = static_cast<int>(ug_base::SampleClock() - AudioMaster()->BlockStartClock());

#if defined( _DEBUG )
	clientClockAtMyBlockStart = clientSampleClock - buffer_offset * oversampleFactor_;
#endif

	clientClockOffset_ = clientSampleClock - ug_base::SampleClock() * oversampleFactor_;

	// Update oversampler out.
	oversampler_out->oversamplerBlockPos = buffer_offset;
	oversampler_in->oversamplerBlockPos = buffer_offset;
	oversampler_in->OnOversamplerResume();

	// reset static counts.
}

void ug_oversampler::DoProcess( int buffer_offset, int sampleframes )
{
#if defined( _DEBUG )
	// Start of new block?
	if( buffer_offset == 0 )
	{
		clientClockAtMyBlockStart = clientSampleClock;
		//_RPT1(_CRT_WARN, "\n---------------- clientClockAtMyBlockStart %d\n", clientClockAtMyBlockStart );
		assert( clientClockOffset_ == clientSampleClock - ug_base::SampleClock() * oversampleFactor_ );
	}
#endif

	// this is needed to accuratly calculate where the client is within my buffer for each sub-block;
	bufferPositionOversampled = buffer_offset * oversampleFactor_;

	oversampler_in->oversamplerBlockPos = buffer_offset;
	oversampler_out->oversamplerBlockPos = buffer_offset;

	//_RPT4(_CRT_WARN, "SampleClock()             %6d   %6d (OS)  | offset %3d, frames %3d\n", ug_base::SampleClock() , oversampler_in->SampleClock(), buffer_offset, sampleframes);
	assert( oversampler_in->SampleClock() == clientSampleClock );

	timestamp_t endTime = ug_base::SampleClock() + sampleframes;

	// Undersampling a single sample FAILS, we don't have access to future events whos timestamp gets rounded down to the same sub-sample.
	// Event ends up getting passed on next block with timstamp too old (usually 1-sample old).
	// simpler to just pass all outstanding events?.
	while( !events.empty() && events.front()->timeStamp < endTime )
	{
		SynthEditEvent* e = events.front();
		events.pop_front();

		switch( e->eventType )
		{
		case UET_EVENT_STREAMING_START:
		case UET_EVENT_STREAMING_STOP:
		case UET_EVENT_SETPIN:
		case UET_EVENT_MIDI:
		{
			// Oversample timestamp.
			timestamp_t OversampledClock = e->timeStamp * oversampleFactor_ + clientClockOffset_;

			//_RPT0(_CRT_WARN, "EVENT\n" );
			//_RPT2(_CRT_WARN, "e->timeStamp             %6d  (%3d)\n", e->timeStamp, e->timeStamp - AudioMaster()->BlockStartClock() );
			//_RPT2(_CRT_WARN, "e->timeStamp (OS)        %6d  (%3d)\n", OversampledClock, OversampledClock - clientClockAtMyBlockStart );

			if( InsidePlugs[e->parm1] )
			{
				//if (e->eventType == UET_EVENT_STREAMING_STOP)
				//{
				//	_RPT1(_CRT_WARN, "ug_oversampler STREAMING_STOP %d\n", (int)e->timeStamp);
				//}

				// Replace outer plug index with inner plug index.
				e->parm1 = InsidePlugs[e->parm1]->getPlugIndex();
				// oversample timestamp.
				e->timeStamp = OversampledClock;
				assert( e->timeStamp >= clientSampleClock );
				// oversampler_in->RelayIncommingEvent( InsidePlugs[e->parm1], e );
				oversampler_in->AddEvent(e);
				e = 0; // prevent deletion.
			}
		}
		break;

		case UET_RUN_FUNCTION2:
		{
			assert( e->timeStamp >= ug_base::SampleClock() );
			if( e->timeStamp == ug_base::SampleClock() )
			{
				ug_base::HandleEvent(e);
			}
		}
		break;

		case UET_GRAPH_START:
			break;

		default:
			assert( false && "unhandled event type" );
		}

		delete_SynthEditEvent(e);
	}

//#if defined( _DEBUG )
//	bool check_blocks = false; // testing true;
//#endif
	int clientSampleFrames = sampleframes * oversampleFactor_;

	int clientBlockPosition = clientSampleClock % BlockSize();

	while( clientSampleFrames > 0 )
	{
		int subSampleFrames = min(clientSampleFrames, BlockSize() - clientBlockPosition);
		// The correct timestamp to insert a module is the start of next 'sweep'.
		moduleInsertClock_ += subSampleFrames;
		
// nope		assert(moduleInsertClock_ == NextGlobalStartClock()); // if this is always true, can remove moduleInsertClock_

		processModules(
			clientBlockPosition
			, subSampleFrames
#ifdef _DEBUG
			, clientSampleClock
#endif
		);

		clientSampleClock += subSampleFrames;
		clientSampleFrames -= subSampleFrames;
		clientBlockPosition += subSampleFrames;

		if( clientBlockPosition >= BlockSize() )
		{
			assert( clientBlockPosition == BlockSize() ); // can't process off-end of a block.
			clientBlockPosition = 0;
			clientBlockStartClock += BlockSize();
			clientNextGlobalStartClock_ += BlockSize();
		}

		// Using running total of client samples to accuratly calculate my buffer position.
		{
			bufferPositionOversampled += subSampleFrames;
			int myBufferPosition = bufferPositionOversampled / oversampleFactor_;

			oversampler_in->oversamplerBlockPos = myBufferPosition;
			oversampler_out->oversamplerBlockPos = myBufferPosition;
		}
	}

	// Update my sample clock.
	EventProcessor::SetSampleClock( EventProcessor::SampleClock() + sampleframes );
	current_run_ug = 0;
}

void ug_oversampler::onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state)
{
}

void ug_oversampler::HandleEvent(SynthEditEvent* e)
{
	switch( e->eventType )
	{
	case UET_GRAPH_START:
			SendPinsInitialStatus();
		break;

	default:
		ug_base::HandleEvent(e);
	}
}

void ug_oversampler::SendPinsInitialStatus()
{
	//for( int c = plugs.GetUpperBound() ; c >= 0 ; --c )
	//{
	//	UPlug* p = plugs[c];
	for( vector<UPlug*>::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
	{
		UPlug* p = *it;

		if( p->DataType == DT_FSAMPLE && p->Direction == DR_OUT)
		{
			p->TransmitState( 0, ST_RUN );
		}
	}
}

// HOST OVERRIDES.
int ug_oversampler::BlockSize()
{
	return AudioMaster()->BlockSize();
}

timestamp_t ug_oversampler::NextGlobalStartClock()
{
	return clientNextGlobalStartClock_;
}

ISeShellDsp* ug_oversampler::getShell()
{
	return AudioMaster()->getShell();
}

void ug_oversampler::InsertBlockDev(EventProcessor* p_module)
{
	//	_RPT2(_CRT_WARN, "inserting ug (%x) into activeModules (sort%d)\n", p_module, so );
#ifdef _DEBUG
	bool is_container = dynamic_cast<ug_base*>( p_module ) != 0;
	assert( p_module->SortOrder2 >= 0 || is_container); // sort order set??
#endif

	activeModules.insertSorted(p_module);

	bool missedExecution = {};
	missedExecution = current_run_ug && current_run_ug->SortOrder2 > p_module->SortOrder2;

	// if prev in list has already executed, new module will miss out till next block.
	if (missedExecution)
	{
		p_module->SetClockRescheduleEvents( moduleInsertClock_ ); // NextGlobalStartClock() );
	}
	else
	{
		p_module->SetClockRescheduleEvents( SampleClock() );
	}
}

void ug_oversampler::SuspendModule(ug_base* p_module)
{
	activeModules.erase( p_module );
}

void ug_oversampler::Wake(EventProcessor* ug)
{
	assert( (ug->flags & UGF_SUSPENDED) == 0 );

	if( ug->sleeping ) // may not be
	{
		// can't do, but may be nesc for emulated ugs:	timestamp_t bsc = ug->m_audio_master->Block StartClock(); // avoid overloaded emulator block stat clock
		// done in InsertBlockDev:	ug->SetSampleClock( Block StartClock() );
		InsertBlockDev( ug );
	}
}

void ug_oversampler::RegisterDspMsgHandle(dsp_msg_target* p_object, int p_handle)
{
	return AudioMaster()->RegisterDspMsgHandle(p_object, p_handle);
}
void ug_oversampler::UnRegisterDspMsgHandle(int p_handle)
{
	return AudioMaster()->UnRegisterDspMsgHandle(p_handle);
}

void ug_oversampler::AssignTemporaryHandle(dsp_msg_target* p_object)
{
	return AudioMaster()->AssignTemporaryHandle(p_object);
}

dsp_msg_target* ug_oversampler::HandleToObject(int p_handle)
{
	return AudioMaster()->HandleToObject(p_handle);
}

void ug_oversampler::ugCreateSharedLookup(const std::wstring& id, LookupTable** lookup_ptr, int sample_rate, size_t p_size, bool integer_table, bool create, SharedLookupScope scope )
{
	AudioMaster()->ugCreateSharedLookup( id, lookup_ptr, sample_rate, p_size, integer_table, create, scope );
}

float ug_oversampler::SampleRate()
{
//	if( isOverSampling_ )
	{
		return AudioMaster()->SampleRate() * oversampleFactor_;
	}
	//else
	//{
	//	return AudioMaster()->SampleRate() / oversampleFactor_;
	//}
}

timestamp_t ug_oversampler::SampleClock()
{
	return clientSampleClock;
}

int ug_oversampler::getBlockPosition()
{
	return clientSampleClock % BlockSize();
}

timestamp_t ug_oversampler::BlockStartClock()
{
	return clientBlockStartClock;
}

int ug_oversampler::latencyCompensationMax()
{
	return AudioMaster()->latencyCompensationMax() * oversampleFactor_;
}

int ug_oversampler::CallVstHost(int opcode, int index, int value, void* ptr, float opt)
{
	return AudioMaster()->CallVstHost(opcode, index, value, ptr, opt);
}

//void ug_oversampler::MasterReset()
//{
//	AudioMaster()->MasterReset();
//}
	
void ug_oversampler::SetModuleLatency(int32_t handle, int32_t latency)
{
	AudioMaster()->SetModuleLatency(handle, latency);
}

void ug_oversampler::onFadeOutComplete()
{
	AudioMaster()->onFadeOutComplete();
}

void ug_oversampler::ServiceGuiQue()
{
	assert(false);
}

void ug_oversampler::RegisterPatchAutomator( class ug_patch_automator* client )
{
	AudioMaster()->RegisterPatchAutomator(client);
}

void ug_oversampler::OnCpuMeasure()
{
	SumCpu();
	CpuFunc();
}

void ug_oversampler::RelayOutGoingEvent( UPlug* outerPlug, SynthEditEvent* e )
{
	timestamp_t clock;

//	assert( isOverSampling_ );
	{
		clock = upsampleTimestamp(e->timeStamp); //  -clientClockOffset_ ) / oversampleFactor_;
		assert( clock == AudioMaster()->BlockStartClock() + (e->timeStamp - clientClockAtMyBlockStart) / oversampleFactor_ );
	}

	switch( e->eventType )
	{
		case UET_EVENT_STREAMING_START:
		case UET_EVENT_STREAMING_STOP:
		{
			state_type new_state = ST_ONE_OFF;

			if( e->eventType == UET_EVENT_STREAMING_START )
			{
				new_state = ST_RUN;
			}

			outerPlug->TransmitState( clock, new_state );
		}
		break;

		case UET_EVENT_SETPIN:
		case UET_EVENT_MIDI:
		{
			outerPlug->Transmit( clock , e->parm2, e->Data() );
		}
		break;
	}
}

void ug_oversampler::RelayOutGoingState( timestamp_t timestamp, UPlug* outerPlug, state_type new_state )
{
	timestamp_t clock;

	//	ensure oversampler isn't using fractions of the outside clock-rate.
	{
		clock = ( timestamp - clientClockOffset_ ) / oversampleFactor_;
#ifdef _DEBUG
		auto clockOld = AudioMaster()->BlockStartClock() + (timestamp - clientClockAtMyBlockStart) / oversampleFactor_;
		assert( clock == clockOld);
#endif
	}

	outerPlug->TransmitState( clock, new_state );
}

timestamp_t ug_oversampler::CalculateOversampledTimestamp( ug_container* top_container, timestamp_t timestamp )
{
//	assert( isOverSampling_ );

//	assert(clientClockOffset_ == clientSampleClock - ug_base::SampleClock() * oversampleFactor_);
// problematic with MIDI-CV not sure why.	assert(clientClockOffset_ == clientClockAtMyBlockStart - ug_base::SampleClock() * oversampleFactor_);
	
	timestamp = timestamp * oversampleFactor_ + clientClockOffset_;

	return ParentContainer()->CalculateOversampledTimestamp( top_container, timestamp );
}

void ug_oversampler::SetPPVoiceNumber(int n)
{
	ug_base::SetPPVoiceNumber(n);
	main_container->SetPPVoiceNumber(n);
}

int ug_oversampler::calcDelayCompensation()
{
	if (cumulativeLatencySamples != LATENCY_NOT_SET)
	{
		return cumulativeLatencySamples;
	}

	cumulativeLatencySamples = ug_base::calcDelayCompensation() + oversampler_out->calcDelayCompensation() / oversampleFactor_;
	return cumulativeLatencySamples;
}

void ug_oversampler::IterateContainersDepthFirst(std::function<void(ug_container*)>& f)
{
	main_container->IterateContainersDepthFirst(f);
}
