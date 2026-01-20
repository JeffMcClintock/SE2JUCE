#pragma once
#include "ug_base.h"
#include "./modules/se_sdk3/mp_api.h"
#include "my_input_stream.h"
#include "conversion.h"
#include "GmpiApiAudio.h"

class ug_plugin3Base :
	public ug_base, public gmpi::IMpHost, public gmpi::IGmpiHost, public gmpi::IEmbeddedFileSupport
{
public:
	ug_plugin3Base(){}

	// ug_base overides
	virtual ug_base* Clone(CUGLookupList& UGLookupList) override;

#if defined( _DEBUG )
	virtual void DebugPrintName();
#endif

	// IUnknown methods
	//GMPI_QUERYINTERFACE1(gmpi::MP_IID_HOST, gmpi::IMpHost)
	int32_t queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override
	{
		*returnInterface = 0;
		if(iid == gmpi::MP_IID_HOST || iid == gmpi::MP_IID_UNKNOWN)
		{
			*returnInterface = static_cast<gmpi::IMpHost*>(this);
			addRef();
			return gmpi::MP_OK;
		}
		if(iid == gmpi::MP_IID_PROCESSOR_HOST)
		{
			*returnInterface = static_cast<gmpi::IGmpiHost*>(this);
			addRef();
			return gmpi::MP_OK;
		}
		if (iid == gmpi::MP_IID_HOST_EMBEDDED_FILE_SUPPORT)
		{
			*returnInterface = static_cast<gmpi::IEmbeddedFileSupport*>(this);
			addRef();
			return gmpi::MP_OK;
		}
		return gmpi::MP_NOSUPPORT;
	}
	GMPI_REFCOUNT_NO_DELETE

	// IMpHost methods
	int32_t setPin(int32_t blockRelativeTimestamp, int32_t pinIndex, int32_t size, const void* data) override;
	int32_t setPinStreaming(int32_t timestamp, int32_t pinIndex, int32_t is_streaming) override;
	int32_t getBlockSize(int32_t& return_val) override;
	int32_t getSampleRate(float& return_val) override;
	int32_t sleep() override;
	int32_t allocateSharedMemory(const wchar_t* table_name, void** table_pointer, float sample_rate, int32_t size_in_bytes, int32_t& ret_need_initialise) override;
	int32_t getHandle(int32_t& return_val) override
	{
		return_val = Handle();
		return gmpi::MP_OK;
	}

	int32_t sendMessageToGui(int32_t id, int32_t size, const void* messageData) override;
	int32_t getHostId(int32_t maxChars, wchar_t* returnString) override;
	int32_t getHostVersion(int32_t& returnValue) override;
	int32_t getRegisteredName(int32_t maxChars, wchar_t* returnString) override;
	int32_t createPinIterator(gmpi::IMpPinIterator** returnInterface) override;
	int32_t isCloned(int32_t* returnValue) override;
	int32_t createCloneIterator(void** returnInterface) override;
	int32_t resolveFilename(const wchar_t* shortFilename, int32_t maxChars, wchar_t* returnFullFilename) override;
	int32_t openProtectedFile(const wchar_t* shortFilename, gmpi::IProtectedFile** file) override;

	// IGmpiHost methods

	// these 2 overlap IMpHost.
	//virtual int32_t setPin( int32_t blockRelativeTimestamp, int32_t pinId, int32_t size, const void* data ) = 0;
	// virtual int32_t sleep() = 0;

	int32_t setPinStreaming(int32_t blockRelativeTimestamp, int32_t pinId, bool isStreaming) override
	{
		return setPinStreaming(blockRelativeTimestamp, pinId, static_cast<int32_t>(isStreaming));
	}
	int32_t setLatency( int32_t latency ) override;
	int32_t getBlockSize() override
	{
		int32_t return_val = 0;
		getBlockSize(return_val);
		return return_val;
	}
	float getSampleRate() override
	{
		float return_val = 0;
		getSampleRate(return_val);
		return return_val;
	}
	int32_t getHandle() override
	{
		int32_t return_val = 0;
		getHandle(return_val);
		return return_val;
	}

	// IEmbeddedFileSupport
	int32_t resolveFilename(const char* fileName, gmpi::IString* returnFullUri) override;
	int32_t openUri(const char* fullUri, gmpi::IMpUnknown** returnStream) override;

	virtual gmpi::IMpUnknown* GetGmpiPlugin() = 0;

	void BuildHelperModule() override;
	bool PPGetActiveFlag() override;
	void OnBufferReassigned() override
	{
		localBufferOffset_ = -1; // invalidate it so that buffers get re-sent to plugin. (editor changed a default)
	}

protected:
	void setupBuffers(int bufferOffset);
	int localBufferOffset_ = 0;
};

template<class IGmpiPluginType, class IGmpiEventType>
class ug_plugin3 :
	public ug_plugin3Base
{
public:
	ug_plugin3() :
		plugin_(0)
	{
	}

	void AttachGmpiPlugin(IGmpiPluginType* p_plugin)
	{
		plugin_ = p_plugin;
	}

	gmpi::IMpUnknown* GetGmpiPlugin() override
	{
		// has to handle unrelated types from gmpi1 and gmpi2
		return reinterpret_cast<gmpi::IMpUnknown*>( plugin_.get() );
	}

	int Open() override
	{
		ug_base::Open();
		int32_t res2 = plugin_->open();

		// Modules with autoduplicating plugs won't have pins until after open();
		setupBuffers(0);
		return res2;
	}

	void setupBuffers(int bufferOffset)
	{
		// setup audio pin buffers
		for( auto p : plugs )
		{
			if( p->DataType == DT_FSAMPLE )
			{
				/* confuses ID with index (and relies on hacks in ug_base::Setup() to make it work)
				plugin_->setBuffer(p->UniqueId(), p->GetSamplePtr() + bufferOffset);
				*/
				plugin_->setBuffer(p->getPlugIndex(), p->GetSamplePtr() + bufferOffset);
			}
		}

		localBufferOffset_ = bufferOffset;
	}

	bool BypassPin(UPlug* fromPin, UPlug* toPin) override
	{
		ug_plugin3Base::BypassPin(fromPin, toPin);
		
		/* confuses ID with index (and relies on hacks in ug_base::Setup() to make it work)
		plugin_->setBuffer(toPin->UniqueId(), fromPin->GetSamplePtr() + localBufferOffset_);
		*/
		
		plugin_->setBuffer(toPin->getPlugIndex(), fromPin->GetSamplePtr() + localBufferOffset_);
		return true;
	}

	void DoProcess(int buffer_offset, int sampleframes) override
	{
		timestamp_t current_sample_clock = SampleClock();
		timestamp_t end_time = current_sample_clock + sampleframes;
		SynthEditEvent* head = nullptr;

		if( !events.empty() )
		{
			SynthEditEvent* prev = 0;
			SynthEditEvent* e2;

			do
			{
				e2 = events.front();

				if( e2->timeStamp < end_time ) // events with same time must be added in same order received (for stat changes to work properly)
				{
					events.pop_front();

					switch( e2->eventType )
					{
//					case UET_DEACTIVATE_VOICE:
					case UET_SUSPEND:
					{
						ug_base::HandleEvent(e2);
					}
					break;
					};

					// set up linked list for plugin
					if( head )
					{
						prev->next = e2;
					}
					else
					{
						head = e2;
					}

					prev = e2;
					// transform to block-relative timing
					int32_t block_relative_clock = (int32_t)( e2->timeStamp - current_sample_clock );
					e2->timeDelta = block_relative_clock;
				}
				else
				{
					e2 = 0;
				}
			} while( !events.empty() && e2 );

			// last pointer zero
			if( prev )
			{
				prev->next = 0;
			}
		}

		// if Host has sliced up buffer, need to reset plugin's buffer pointers.
		if( buffer_offset != localBufferOffset_ )
		{
			setupBuffers(buffer_offset);
		}

		/*
		#if defined( _DEBUG )
		e2 = head;
		while( e2 )
		{
		if( e2->eventType == UET_EVENT_SETPIN )
		_RPT3(_CRT_WARN, "event:SETPIN,%d,%d,%f\n", e2->parm1, e2->parm2, *(float*)&(e2->parm3) );
		else
		_RPT4(_CRT_WARN, "event:%d,%d,%d,%f\n", e2->eventType, e2->parm1, e2->parm2, *(float*)&(e2->parm3) );

		e2 = e2->next;
		}
		#endif
		*/

		plugin_->process(sampleframes, reinterpret_cast<IGmpiEventType*>(static_cast<gmpi::MpEvent*>(head)));
		SetSampleClock(end_time);

		while(head)
		{
			SynthEditEvent* temp = head;
			head = (SynthEditEvent*)head->next;

			delete_SynthEditEvent(temp);
		}
	}

	void OnUiNotify2(int p_msg_id, my_input_stream& p_stream) override
	{
		ug_base::OnUiNotify2(p_msg_id, p_stream);

		if( p_msg_id == id_to_long("sdk") )
		{
			int size, user_msg_id;
			p_stream >> user_msg_id;
			p_stream >> size;
			void* data = malloc(size);
			p_stream.Read(data, size);

			receiveMessageFromGui(user_msg_id, size, data);

			free(data);
		}
	}

	void receiveMessageFromGui(int32_t id, int32_t size, const void* data)
	{
		auto clone = this;

		while (clone)
		{
			// wake it
			// What if it's suspended???!!! message sent anyhow. what if it trys to do somthing?
			if ((clone->flags & UGF_SUSPENDED) == 0 && clone->sleeping)
			{
				clone->Wake();
			}

			clone->plugin_->receiveMessageFromGui(id, size, const_cast<void*>(data));
			clone = (ug_plugin3<IGmpiPluginType, IGmpiEventType>*) clone->m_next_clone;
		}
	}

	static ug_base * CreateObject(){ return new ug_plugin3<IGmpiPluginType, IGmpiEventType>(); }
	ug_base * Create() override{ return CreateObject(); }

	gmpi_sdk::mp_shared_ptr<IGmpiPluginType> plugin_;
};

// specializations
template<>
inline void ug_plugin3<gmpi::IMpAudioPlugin, gmpi::MpEvent>::receiveMessageFromGui(int32_t /*id*/, int32_t /*size*/, const void* /*messageData*/)
{
	// not supported, but could be done with an extension method 'IMessageSender' or whatever
}
template<>
inline void ug_plugin3<gmpi::api::IProcessor, gmpi::api::Event>::receiveMessageFromGui(int32_t /*id*/, int32_t /*size*/, const void* /*messageData*/)
{
	// not supported, but could be done with an extension method 'IMessageSender' or whatever
}

// specialization. GMPI needs host passed in Open
template<>
inline int ug_plugin3<gmpi::api::IProcessor, gmpi::api::Event>::Open()
{
	ug_base::Open();
	const int32_t res2 = (int32_t) plugin_->open(reinterpret_cast<gmpi::api::IUnknown*>(static_cast<gmpi::IMpHost*>(this)));

	// Modules with autoduplicating plugs won't have pins until after open();
	setupBuffers(0);
	return res2;
}
