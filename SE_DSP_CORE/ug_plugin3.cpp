#include "pch.h"
// Fix for <sstream> on Mac (sstream uses undefined int_64t)
#include "modules/se_sdk3/mp_sdk_common.h"
#include <sstream>
#include "./ug_plugin3.h"
#include "SeAudioMaster.h"
#include "iseshelldsp.h"
#include "my_msg_que_output_stream.h"
#include "module_info.h"
#include "ULookup.h"
#include "version.h"
#include "Sdk3CloneIterator.h"
#include "./modules/shared/xplatform.h"
#include "conversion.h"
#include "UgDatabase.h"
#include "ug_container.h"
#include "MpPinIterator.h"
#include "ProtectedFile.h"

using namespace std;
/* TODO.

sending value twice on same sampleclock to delete old event
*/

ug_base* ug_plugin3Base::Clone( CUGLookupList& UGLookupList )
{
	// Deliberately don't call addplugs(), as this is done by calling func
	ug_base* clone = moduleType->BuildSynthOb();
	SetupClone(clone);

	if (GetFlag(UGF_HAS_HELPER_MODULE))
	{
		/*
		if (moduleType->UniqueId() == L"SE Poly to Mono Helper")
		{
			// Connect to voice-0 Poly to Mono.
			auto to = clone->GetPlug(2);
			auto fromUg = CloneOf()->GetPlug(0)->connections.front()->UG;

			while (fromUg->m_next_clone) // all except last (which gets connected via usual method).
			{
				// sneakly illegal connection without adder. Used to transmit last-voice-number in a many-to-many pattern.
				fromUg->GetPlug(2)->connections.push_back(to);
				fromUg = fromUg->m_next_clone;
			}

			auto toUg = CloneOf();
			while (toUg != clone) // all except last (which gets connected via usual method).
			{
				// sneakly illegal connection without adder. Used to transmit last-voice-number in a many-to-many pattern.
				fromUg->GetPlug(2)->connections.push_back(toUg->GetPlug(2));
				toUg = toUg->m_next_clone;
			}
		}
		*/
	}

	return clone;
}

// set an output pin
int32_t ug_plugin3Base::setPin(int32_t blockRelativeTimestamp, int32_t id, int32_t size, const void* data)
{
	auto pin = GetPlugById(id);

	if(!pin)
		return gmpi::MP_FAIL;

	if (pin->Direction != DR_OUT)
	{
		std::wostringstream oss;
		oss << L"Error: " << moduleType->GetName() << L" sending data out INPUT pin.";
		AudioMaster2()->ugmessage( oss.str().c_str());
		return gmpi::MP_FAIL;
	}

	if( blockRelativeTimestamp < 0 )
	{
		std::wostringstream oss;
		oss << L"Error: " << moduleType->GetName() << L" sending data with negative timestamp. Please inform module vendor.";
		AudioMaster2()->ugmessage( oss.str().c_str());
		return gmpi::MP_FAIL;
	}

	timestamp_t timestamp = blockRelativeTimestamp + AudioMaster()->BlockStartClock();
	timestamp += localBufferOffset_;
	assert(timestamp == blockRelativeTimestamp + SampleClock()); // function below uses this alternate caluation. Which is it?

	pin->Transmit(timestamp, size, data);
	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::setPinStreaming( int32_t blockRelativeTimestamp, int32_t id, int32_t is_streaming)
{
	auto pin = GetPlugById(id);

	if (!pin)
		return gmpi::MP_FAIL;

	if(pin->Direction != DR_OUT)
	{
		AudioMaster2()->ugmessage(moduleType->GetName() + L" Error: DSP Module setting streaming on INPUT pin");
		return gmpi::MP_FAIL;
	}

	// not in VST mode.	assert( SampleClock() == AudioMaster()->BlockStartClock() );
	// todo: all timestamps block-relative
	timestamp_t abs_timestamp = blockRelativeTimestamp + SampleClock();
	// FL Click bug. This seems wrong as SampleClock() already includes localBufferOffset_.
	// abs_timestamp += localBufferOffset_;
	state_type s;

	if( is_streaming == 0 )
	{
		s = ST_STATIC;
	}
	else
	{
		s = ST_RUN;
	}

	pin->TransmitState( abs_timestamp, s );
	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::getBlockSize(int32_t& return_val)
{
	return_val = AudioMaster()->BlockSize();
	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::getSampleRate(float& return_val)
{
	return_val = AudioMaster()->SampleRate();
	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::sleep()
{
	//_RPT2(_CRT_WARN, " ug_plugin3Base::sleep %x %d\n", this, SampleClock() );
	SleepMode();
	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::allocateSharedMemory( const wchar_t* table_name, void** table_pointer, float sample_rate, int32_t size_in_bytes, int32_t& ret_need_initialise )
{
	ret_need_initialise = false;
	ULookup* lu = 0;
	*table_pointer = 0;
	int size_in_floats = (size_in_bytes+3) / 4;
//	CreateSharedLookup2( table_name, lu, (int) sample_rate, size_in_floats, true, SLS_ONE_MODULE );
	// Size <= 0 indicates not to create it, just locate existing.
	CreateSharedLookup2( table_name, lu, (int) sample_rate, size_in_floats, size_in_bytes > 0, SLS_ONE_MODULE );

	if( lu == 0 )
	{
		// OK to 'fail' if only a query (size_in_floats == -1 ).
		return size_in_floats != -1 ? gmpi::MP_FAIL : gmpi::MP_OK;
	}

	// note: this must be checked before SetInitialised()
	ret_need_initialise = !lu->GetInitialised();

	if( !lu->GetInitialised() )
	{
#if 0 // defined( _DEBUG )
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
//	return lu->table == 0 ? gmpi::MP_FAIL : gmpi::MP_OK;
	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::sendMessageToGui( int32_t id, int32_t size, const void* messageData )
{
	auto queue = AudioMaster()->getShell()->MessageQueToGui();
	
	// discard any too-big message.
	const auto totalMessageSize = 4 * static_cast<int>(sizeof(int)) + size;
	if(totalMessageSize > queue->freeSpace())
		return gmpi::MP_FAIL;

	my_msg_que_output_stream strm( queue, Handle(), "sdk");
	
	strm << (int) ( sizeof(int) + sizeof(int) + size ); // message length.
	strm << id;											// user provided msg_id
	strm << (int) size;									// Send block size

	strm.Write( messageData, size );	// send block
	strm.Send();

	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::getHostId( int32_t maxChars, wchar_t* returnString )
{
#if defined( _MSC_VER )
	wcscpy_s( returnString, maxChars, L"SynthEdit" );
#else
	wcscpy( returnString, L"SynthEdit" );
#endif
	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::getHostVersion( int32_t& returnValue )
{
	returnValue = EXE_VERSION_NUM;
	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::getRegisteredName( int32_t maxChars, wchar_t* returnString )
{
	std::wstring user, t;
	AudioMaster()->getShell()->GetRegistrationInfo(user, t);
#if defined( _MSC_VER )
	wcscpy_s( returnString, maxChars, (user).c_str() );
#else
	wcscpy( returnString, (user).c_str() );
#endif
	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::createPinIterator(gmpi::IMpPinIterator** returnIterator )
{
	if( !GetFlag(UGF_OPEN) )
	{
		AudioMaster2()->ugmessage( moduleType->GetName() + L": Can't iterate pins in constructor. use open() method instead.");
	}

	gmpi_sdk::mp_shared_ptr<gmpi::IMpPinIterator> ob;
	ob.Attach( new MpPinIterator(this) );
	return ob->queryInterface( gmpi::MP_IID_UNKNOWN, (void**) returnIterator);
}

int32_t ug_plugin3Base::isCloned( int32_t* returnValue )
{
	*returnValue = GetPolyphonic();
	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::createCloneIterator( void** returnInterface )
{
	*returnInterface = new Sdk3CloneIterator(this);

	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::resolveFilename( const wchar_t* shortFilename, int32_t maxChars, wchar_t* returnFullFilename )
{
//	_RPT0(_CRT_WARN, "seaudioMasterResolveFilename)\n" );

	std::wstring filename = std::wstring(shortFilename);
	std::wstring f_ext = GetExtension(filename);
	std::wstring full_filename = AudioMaster()->getShell()->ResolveFilename( filename, f_ext );
	WStringToWchars( full_filename, returnFullFilename, (int) maxChars );

	return gmpi::MP_OK;
}

int32_t ug_plugin3Base::openProtectedFile( const wchar_t* shortFilename, gmpi::IProtectedFile** file )
{
	*file = nullptr; // If anything fails, return 0.

	// new way, disk files only. (and no MFC).
	std::wstring filename(shortFilename);
	std::wstring l_filename = AudioMaster()->getShell()->ResolveFilename(filename, L"");
	auto filename_utf8 = WStringToUtf8(l_filename);

	auto pf2 = ProtectedFile2::FromUri(filename_utf8.c_str());

	if (pf2)
	{
		*file = pf2;
		return gmpi::MP_OK;
	}

	return gmpi::MP_FAIL;
}

int32_t ug_plugin3Base::setLatency(int32_t latency)
{
	AudioMaster()->SetModuleLatency(Handle(), latency);
	return gmpi::MP_OK;
}

#if defined( _DEBUG )
void ug_plugin3Base::DebugPrintName()
{
	_RPTW1(_CRT_WARN, L"%s ", moduleType->GetName().c_str() );
}
#endif

void ug_plugin3Base::BuildHelperModule()
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

bool ug_plugin3Base::PPGetActiveFlag()
{
	if (GetPPDownstreamFlag())
	{
		// important to prevent voices being held open thru unterminated patch points.
		if (getModuleType()->UniqueId() == L"SE Patch Point out")
		{
			// not suffcient
// bug if patch point connected from last polyphonic modules (e.g. VCA)			SetFlag(UGF_VOICE_MON_IGNORE);

			if (plugs[1]->connections.empty()) // unconnected patch-point.
			{
				SetFlag(UGF_VOICE_MON_IGNORE);
/*
			// also not sufficient
				// Assume upstream module is polyphonic if input pin is flagged.
				if (plugs[0]->GetPPDownstreamFlag())
				{
					// this module will be polyphonic, and not cause unwanted voice-monitor to be inserted upstream.
					return true;
				}
*/
			}
		}
	}

	return ug_base::PPGetActiveFlag();
}

int32_t ug_plugin3Base::resolveFilename(const char* fileName, gmpi::IString* returnFullUri)
{
	const std::string resourceNameStr(fileName);
	const auto filenameW = Utf8ToWstring(resourceNameStr);
	auto f_ext = GetExtension(filenameW);

	const auto full_filename = AudioMaster()->getShell()->ResolveFilename( filenameW, f_ext );
	const auto full_filenameU = WStringToUtf8(full_filename);

	return returnFullUri->setData(full_filenameU.data(), (int32_t) full_filenameU.size());
}

int32_t ug_plugin3Base::openUri(const char* fullUri, gmpi::IMpUnknown** returnStream)
{
	*returnStream = static_cast<gmpi::IMpUnknown*>(ProtectedFile2::FromUri(fullUri));

	return *returnStream ? gmpi::MP_OK : gmpi::MP_FAIL;
}


////////////////////////////////////////////////////////////////////////////

// This is here to save creating extra .cpp file for a a couple of functions.
Sdk3CloneIterator::Sdk3CloneIterator(class ug_base* ug) :
current_(0)
,first_(ug->m_clone_of)
{
}

int32_t Sdk3CloneIterator::next(gmpi::IMpUnknown** returnInterface )
{
	*returnInterface = 0;

    if( current_ )
    {
        *returnInterface = dynamic_cast<ug_plugin3Base*>(current_)->GetGmpiPlugin();
        current_ = current_->m_next_clone;
		return gmpi::MP_OK;
    }
	return gmpi::MP_FAIL;
}
