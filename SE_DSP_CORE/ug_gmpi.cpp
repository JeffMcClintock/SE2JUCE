#include "pch.h"
#include <sstream>
#include "./ug_gmpi.h"
#include "module_info.h"
#include "SeAudioMaster.h"
#include "CVoiceList.h"
#include "ProtectedFile.h"


ug_gmpi::ug_gmpi(class Module_Info* p_moduleType, gmpi::api::IProcessor* p_plugin) : plugin_(p_plugin)
{
	setModuleType(p_moduleType);
}

ug_base* ug_gmpi::Clone( CUGLookupList& /*UGLookupList*/ )
{
	auto clone = moduleType->BuildSynthOb();
	SetupClone(clone);
	return clone;
}

// set an output pin
gmpi::ReturnCode ug_gmpi::setPin( int32_t blockRelativeTimestamp, int32_t id, int32_t size, const void* data )
{
	if (plugs[id]->Direction != DR_OUT)
	{
		std::wostringstream oss;
		oss << L"Error: " << moduleType->GetName() << L" sending data out INPUT pin.";
		AudioMaster2()->ugmessage( oss.str().c_str());
		return gmpi::ReturnCode::Fail;
	}

	if( blockRelativeTimestamp < 0 )
	{
		std::wostringstream oss;
		oss << L"Error: " << moduleType->GetName() << L" sending data with negative timestamp. Please inform module vendor.";
		AudioMaster2()->ugmessage( oss.str().c_str());
		return gmpi::ReturnCode::Fail;
	}

	timestamp_t timestamp = blockRelativeTimestamp + AudioMaster()->BlockStartClock();
	timestamp += localBufferOffset_;
	assert(timestamp == blockRelativeTimestamp + SampleClock()); // function below uses this alternate caluation. Which is it?

	plugs[id]->Transmit(timestamp, size, data);
	return gmpi::ReturnCode::Ok;
}

gmpi::ReturnCode ug_gmpi::setPinStreaming( int32_t blockRelativeTimestamp, int32_t id, bool is_streaming)
{
	auto pin = plugs[id];

	if(pin->Direction != DR_OUT)
	{
		AudioMaster2()->ugmessage(moduleType->GetName() + L" Error: DSP Module setting streaming on INPUT pin");
		return gmpi::ReturnCode::Fail;
	}

	const timestamp_t abs_timestamp = blockRelativeTimestamp + SampleClock();

	pin->TransmitState( abs_timestamp, is_streaming ? ST_RUN : ST_STATIC);

	return gmpi::ReturnCode::Ok;
}

gmpi::ReturnCode ug_gmpi::sleep()
{
	SleepMode();
	return gmpi::ReturnCode::Ok;
}

void ug_gmpi::BuildHelperModule()
{
	if ( GetFlag(UGF_HAS_HELPER_MODULE) )
	{
		if ( GetFlag(UGF_POLYPHONIC_GENERATOR_CLONED) )
		{
			CreateMidiRedirector(this);
		}
	}
	else
	{
		ug_base::BuildHelperModule();
	}
}

bool ug_gmpi::PPGetActiveFlag()
{
	if (GetPPDownstreamFlag())
	{
		// important to prevent voices being held open thru unterminated patch points.
		if (getModuleType()->UniqueId() == L"SE Patch Point out")
		{
			if (plugs[1]->connections.empty()) // unconnected patch-point.
			{
				SetFlag(UGF_VOICE_MON_IGNORE);
			}
		}
	}

	return ug_base::PPGetActiveFlag();
}

gmpi::ReturnCode ug_gmpi::resolveFilename(const char* fileName, gmpi::api::IString* returnFullUri) 
{
	const std::string resourceNameStr(fileName);
	const auto filenameW = Utf8ToWstring(resourceNameStr);
	auto f_ext = GetExtension(filenameW);

	const auto full_filename = AudioMaster()->getShell()->ResolveFilename( filenameW, f_ext );
	const auto full_filenameU = WStringToUtf8(full_filename);

	return returnFullUri->setData(full_filenameU.data(), (int32_t) full_filenameU.size());
}

gmpi::ReturnCode ug_gmpi::openUri(const char* fullUri, gmpi::api::IUnknown** returnStream)
{
	*returnStream = reinterpret_cast<gmpi::api::IUnknown*>(static_cast<gmpi::IMpUnknown*>(ProtectedFile2::FromUri(fullUri)));

	return *returnStream ? gmpi::ReturnCode::Ok : gmpi::ReturnCode::Fail;
}

gmpi::ReturnCode ug_gmpi::queryInterface(const gmpi::api::Guid* iid, void** returnInterface)
{
	*returnInterface = {};

	if (*iid == gmpi::api::IProcessorHost::guid || *iid == gmpi::api::IUnknown::guid)
	{
		*returnInterface = static_cast<gmpi::api::IProcessorHost*>(this);
		addRef();
		return gmpi::ReturnCode::Ok;
	}

	if (*iid == synthedit::IEmbeddedFileSupport::guid)
	{
		*returnInterface = static_cast<synthedit::IEmbeddedFileSupport*>(this);
		addRef();
		return gmpi::ReturnCode::Ok;
	}

	return gmpi::ReturnCode::NoSupport;
}

void ug_gmpi::OnBufferReassigned()
{
	localBufferOffset_ = -1; // invalidate it so that buffers get re-sent to plugin. (editor changed a default)
}

//void ug_gmpi::AttachGmpiPlugin(gmpi::api::IProcessor* p_plugin)
//{
//	plugin_ = p_plugin;
//}

void ug_gmpi::setupBuffers(int bufferOffset)
{
	// setup audio pin buffers
	for (auto p : plugs)
	{
		if (p->DataType == DT_FSAMPLE)
		{
			plugin_->setBuffer(p->getPlugIndex(), p->GetSamplePtr() + bufferOffset);
		}
	}

	localBufferOffset_ = bufferOffset;
}

bool ug_gmpi::BypassPin(UPlug* fromPin, UPlug* toPin)
{
	ug_base::BypassPin(fromPin, toPin);

	plugin_->setBuffer(toPin->getPlugIndex(), fromPin->GetSamplePtr() + localBufferOffset_);
	return true;
}

void ug_gmpi::DoProcess(int buffer_offset, int sampleframes)
{
	timestamp_t current_sample_clock = SampleClock();
	timestamp_t end_time = current_sample_clock + sampleframes;
	SynthEditEvent* head = nullptr;

	if (!events.empty())
	{
		SynthEditEvent* prev = 0;
		SynthEditEvent* e2;

		do
		{
			e2 = events.front();

			if (e2->timeStamp < end_time) // events with same time must be added in same order received (for stat changes to work properly)
			{
				events.pop_front();

				switch (e2->eventType)
				{
				case UET_SUSPEND:
				{
					ug_base::HandleEvent(e2);
				}
				break;
				};

				// set up linked list for plugin
				if (head)
				{
					prev->next = e2;
				}
				else
				{
					head = e2;
				}

				prev = e2;
				// transform to block-relative timing
				int32_t block_relative_clock = (int32_t)(e2->timeStamp - current_sample_clock);
				e2->timeDelta = block_relative_clock;
			}
			else
			{
				e2 = 0;
			}
		} while (!events.empty() && e2);

		// last pointer zero
		if (prev)
		{
			prev->next = 0;
		}
	}

	// if Host has sliced up buffer, need to reset plugin's buffer pointers.
	if (buffer_offset != localBufferOffset_)
	{
		setupBuffers(buffer_offset);
	}

	// convert to GMPI events (overwriting SE Events)
	for (auto from = static_cast<gmpi::MpEvent*>(head); from; from = from->next)
	{
		auto to = reinterpret_cast<gmpi::api::Event*>(from);
		
		const auto temp = *from; // copy bits to temp, since we're about to overwrite the event.

		to->next = reinterpret_cast<gmpi::api::Event*>(temp.next);			// first 8 bytes. overwrites timeDelta, eventType.
		to->timeDelta = temp.timeDelta;										// 4 bytes. overwrites parm1.
		to->eventType = static_cast<gmpi::api::EventType>(temp.eventType - 100);	// 4 bytes. overwrites parm2 (size).
		to->pinIdx = temp.parm1;											// 4 bytes. overwrites parm3 (data).
		to->size_ = temp.parm2;												// 4 bytes. overwrites parm4 (data).

		if (!from->extraData)
		{
			auto src = reinterpret_cast<const uint8_t*>(&temp.parm3);
			std::copy(src, src + sizeof(to->data_), to->data_);
		}
#if _DEBUG
		else
		{
			// 'extraData' should alias over the top of 'oversizeData_'
			assert(to->oversizeData_ == (const uint8_t*)(temp.extraData));
			// to->oversizeData_ = reinterpret_cast<uint8_t*>(temp.extraData); // 8 bytes. overwrites extraData.
		}
#endif
	}

	auto head_gmpi = reinterpret_cast<const gmpi::api::Event*>(static_cast<gmpi::MpEvent*>(head));
	plugin_->process(sampleframes, head_gmpi);
	SetSampleClock(end_time);

	while (head)
	{
		auto temp = head;
		head = (SynthEditEvent*)head->next;

		auto gmpi_event = reinterpret_cast<gmpi::api::Event*>(static_cast<gmpi::MpEvent*>(temp));
		/* old
		// extraData is not valid (it's overwritten) so zero it after deleting the memory pointed to by the GMPI event.
		if (gmpi_event->size_ > 8)
		{
			delete[] gmpi_event->oversizeData_;
		}
		temp->extraData = {};
		*/

		// extradata not valid if we used the storage for small data.
		if (gmpi_event->size_ <= 8)
		{
			temp->extraData = {};
		}

		delete_SynthEditEvent(temp);
	}
}

int ug_gmpi::Open()
{
	ug_base::Open();

	const auto res2 = (int32_t)plugin_->open(static_cast<gmpi::api::IProcessorHost*>(this));

	// Modules with autoduplicating plugs won't have pins until after open();
	setupBuffers(0);
	return res2;
}

gmpi::ReturnCode ug_gmpi::setLatency(int32_t latency)
{
	AudioMaster()->SetModuleLatency(Handle(), latency);
	return gmpi::ReturnCode::Ok;
}

int32_t ug_gmpi::getBlockSize()
{
	return AudioMaster()->BlockSize();
}

float ug_gmpi::getSampleRate()
{
	return AudioMaster()->SampleRate();
}

int32_t ug_gmpi::getHandle()
{
	return m_handle;
}

ug_base* ug_gmpi::Create()
{
	assert(false); // should never be called.
	return {}; // new ug_gmpi();
}

#if defined( _DEBUG )
void ug_gmpi::DebugPrintName()
{
	_RPTW1(_CRT_WARN, L"%s ", moduleType->GetName().c_str());
}
#endif

