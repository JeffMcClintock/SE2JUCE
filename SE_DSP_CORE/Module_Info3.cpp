#include "pch.h"

#include "Module_Info3.h"
#include "ug_plugin3.h"
#include "BundleInfo.h"
#include "tinyxml/tinyxml.h"
#include <sstream>
#include "./modules/se_sdk3/MpString.h"
#include "GmpiSdkCommon.h"

#if defined( SE_EDIT_SUPPORT )
#include "./modules/shared/xp_dynamic_linking.h"
#include "SynthEditDocBase.h"
#include "UG2.h"
#include "docob.h"
#include "SynthEditAppBase.h"
#include "../SynthEdit/SafeMessageBox.h"
#endif

#include "mfc_wstring.h"

#if !defined( _WIN32 )
// mac
#include <dlfcn.h>
#endif

using namespace gmpi;
using namespace gmpi_sdk;
using namespace gmpi_dynamic_linking;

IMPLEMENT_SERIAL( Module_Info3, Module_Info, 1 )

Module_Info3::Module_Info3() :// serialisation only
dllHandle(0)
{
}

Module_Info3::Module_Info3( const std::wstring& file_and_dir, const std::wstring& overridingCategory ) :
	filename(file_and_dir)
	,dllHandle(0)
{
	if( !overridingCategory.empty() )
	{
		m_group_name = overridingCategory;
	}
}

Module_Info3::~Module_Info3()
{
	Unload();
}

// Need to serialise in SynthEdit and in VST2 plugins, but not in waves, WinRt, or VST3.
#if defined( SE_EDIT_SUPPORT )
void Module_Info3::Serialize( CArchive& ar )
{
	Module_Info3_base::Serialize( ar );
	if( ar.IsStoring() )
	{
		ar <<  ug_flags;
		ar <<  m_window_type;

		// SEM cache requires full path.
		if( CSynthEditDocBase::serializingMode == SERT_SEM_CACHE )
		{
			ar << filename;
		}
		else
		{
			// Save-as-VST requires only filename (all SEMS unloaded in same folder).
			auto shrtfilename = StripPath(filename);
			ar << shrtfilename;
		}

		ar << m_gui_registered;
		ar << m_dsp_registered;
	}
	else
	{
		ar >>  ug_flags;
		ar >>  m_window_type;
		ar >> filename;
		ar >> m_gui_registered;
		ar >> m_dsp_registered;

		if( CSynthEditDocBase::serializingMode == SERT_SE1_DOC )
		{
			// clear out filename etc because SEM may not exist on local system.
			// we're only loading the module info to allow se1 to display correctly, not to function.
			filename = L"";
			m_gui_registered = false;
			m_dsp_registered = false;
		}
	}
}
#endif

// developer mode only, reload dll if user has updated it
void Module_Info3::ReLoadDll()
{
	Unload();
	SetupPlugs();
}

// returns true if dll not available
bool Module_Info3::LoadDllOnDemand()
{
	if (!dllHandle)
	{
		LoadDll(); // load on demand

#if defined( SE_EDIT_SUPPORT )
		if (dllHandle && isShellPlugin() && isSummary()) // shell plugins need info fleshed out.
		{
			auto factory = getFactory2();

			if( factory != nullptr )
			{
				gmpi_sdk::mp_shared_ptr<gmpi::IMpShellFactory> shellFactory;
				{
					auto r = factory->queryInterface(gmpi::MP_IID_SHELLFACTORY, shellFactory.asIMpUnknownPtr());
					MpString s;
					r = shellFactory->getPluginInformation(m_unique_id.c_str(), s.getUnknown());

					// scan XML.
					if (r != gmpi::MP_OK)
					{
						// Shell does not have this plugin.
						return true; // fail
					}

					{
						TiXmlDocument doc2;
						doc2.Parse(s.c_str());
						
						if( doc2.Error() )
						{
#if defined( SE_EDIT_SUPPORT )
							std::wostringstream oss;
							oss << L"Module XML Error: [SynthEdit.exe]" << doc2.ErrorDesc() << L"." << doc2.Value();
							SafeMessagebox(0, oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);
#else
							assert(false);
#endif
						}
						else
						{
							TiXmlNode* pluginList = doc2.FirstChild("PluginList");
							if (!pluginList)
							{
								return true; // fail
							}

							TiXmlNode* node = pluginList->FirstChild("Plugin");
							if (!node)
							{
								return true; // fail
							}

							// check for existing
							TiXmlElement* PluginElement = node->ToElement();
							std::wstring plugin_id = Utf8ToWstring(PluginElement->Attribute("id"));

							// can be caused by saving the same dll with different unique ID. e.g. by saving PD303 from it's prefab twice. Without re-scanning VSTs.
							if (plugin_id != UniqueId())
							{
								return true; // fail
							}

							scanned_xml_dsp = scanned_xml_gui = false; // prevent spurious warnings.
							ScanXml(PluginElement);
						}
					}
				}
			}
		}
#endif

	}

	return !static_cast<bool>(dllHandle);
}

void Module_Info3::LoadDll()
{
	//	_RPT1(_CRT_WARN, "Module_Info3::LoadDll %s\n", filename );
	std::wstring load_filename = filename;

#if defined( SE_EDIT_SUPPORT )
	CWaitCursor wait;
#else
	#if defined( SE_TARGET_PLUGIN ) && SE_EXTERNAL_SEM_SUPPORT==1
		load_filename = combine_path_and_file( BundleInfo::instance()->getSemFolder(), load_filename );
	#endif
#endif

	// Get a handle to the DLL module.
#if defined( _WIN32)
		if (MP_DllLoad(&dllHandle, load_filename.c_str()))
		{
			// load failed, try it as a bundle.
			const auto bundleFilepath = load_filename + L"/Contents/x86_64-win/" + filename;
			MP_DllLoad(&dllHandle, bundleFilepath.c_str());
		}
#else
    	// int32_t r = MP_DllLoad( &dllHandle, load_filename.c_str() );

		// Create a path to the bundle
		CFStringRef pluginPathStringRef = CFStringCreateWithCString(NULL,
			WStringToUtf8( load_filename ).c_str(), kCFStringEncodingASCII);

		CFURLRef bundleUrl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
		pluginPathStringRef, kCFURLPOSIXPathStyle, true);
		if(bundleUrl == NULL) {
			printf("Couldn't make URL reference for plugin\n");
			return;
		}

		// Open the bundle
		dllHandle = (DLL_HANDLE) CFBundleCreate(kCFAllocatorDefault, bundleUrl);
		if(dllHandle == 0) {
			printf("Couldn't create bundle reference\n");
			CFRelease(pluginPathStringRef);
			CFRelease(bundleUrl);
			return;
		}
    	return; // success.
    #endif

#if defined( _WIN32)
	// If the handle is valid, we're done
	if(dllHandle != NULL)
	{
		return; // OK
	}

	// some problem
	std::wstring errorMessage;
	{
		DWORD err_code = GetLastError();
		LPTSTR lpMsgBuf = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err_code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
		);

		errorMessage = lpMsgBuf;
		LocalFree(lpMsgBuf);
	}

#else
    assert(false);
#endif
    
	// avoid bugging user over and over.
	if(!m_module_dll_available)
	{
		return;
	}

	m_module_dll_available = false;

#if defined( SE_EDIT_SUPPORT )
	std::wostringstream oss;
	oss << errorMessage << L": " << load_filename << L".";
	SafeMessagebox(0, oss.str().c_str(), L"", MB_OK|MB_ICONSTOP );
#else
	assert(false);
#endif
}

void Module_Info3::Unload()
{
#if defined( _WIN32)
	if (dllHandle != NULL)
	{
		const auto r = MP_DllUnload( dllHandle );

#ifdef _DEBUG
		if (r)
		{
			_RPTW1(_CRT_WARN, L"FAIL!: Module_Info3::Unload %s\n", filename.c_str() );
		}
#endif

		dllHandle = 0;
	}
#else
	if( dllHandle != 0 )
	{
    	CFBundleUnloadExecutable((CFBundleRef) dllHandle);
    	CFRelease((CFBundleRef) dllHandle);
    }
#endif

}

gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> Module_Info3::getFactory2()
{
	if (LoadDllOnDemand())
	{
		return {};
	}

	int32_t r;

	gmpi::MP_DllEntry dll_entry_point = {};
#ifdef _WIN32
	const char* gmpi_dll_entrypoint_name = "MP_GetFactory";
	r = MP_DllSymbol(dllHandle, gmpi_dll_entrypoint_name, (void**)&dll_entry_point);
#else
	dll_entry_point = (gmpi::MP_DllEntry)CFBundleGetFunctionPointerForName((CFBundleRef)dllHandle, CFSTR("MP_GetFactory"));
#endif        

	if (!dll_entry_point)
	{
		return {};
	}

	gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> factory;
	r = dll_entry_point(factory.asIMpUnknownPtr());

	return factory;
}

gmpi::IMpUnknown* Module_Info3::Build( int subType, bool quietFail )
{
	auto com_object = getFactory2();

	if(!com_object)
	{
		return {};
	}

	{
		gmpi_sdk::mp_shared_ptr<gmpi::IMpFactory2> factory;
		const auto r = com_object->queryInterface(gmpi::MP_IID_FACTORY2, factory.asIMpUnknownPtr());
		if (r == gmpi::MP_OK && factory)
		{
			// STEP 4: Ask factory to instantiate plugin.
			gmpi::IMpUnknown* com_object2 = 0;
			int32_t createResult = factory->createInstance2(m_unique_id.c_str(), subType, reinterpret_cast<void**>(&com_object2));

			//				ERROR_CHECK(r,"fail to create plugin\n");
			if (createResult == gmpi::MP_OK)
			{
				return com_object2;
			}
		}
	}

	{
		// GMPI IPluginFactory uses a utf8 ID (not wstring).
		auto gmpi_com_object = (gmpi::api::IUnknown*)com_object.get();
		gmpi::shared_ptr<gmpi::api::IPluginFactory> factory;
		const auto r = gmpi_com_object->queryInterface(&gmpi::api::IPluginFactory::guid, factory.asIMpUnknownPtr());
		if (r == gmpi::ReturnCode::Ok && factory)
		{
			const auto uniqueIdUtf8 = WStringToUtf8(m_unique_id);
			// STEP 4: Ask factory to instantiate plugin.
			gmpi::api::IUnknown* com_object2{};
			const auto createResult = factory->createInstance(uniqueIdUtf8.c_str(), (gmpi::api::PluginSubtype) subType, reinterpret_cast<void**>(&com_object2));

			// ERROR_CHECK(r,"fail to create plugin\n");
			if (createResult == gmpi::ReturnCode::Ok)
			{
				return (gmpi::IMpUnknown*) com_object2; // these are compatible enough
			}
		}
	}

	if( !quietFail )
	{
#if defined( SE_EDIT_SUPPORT )
		std::wostringstream oss;
		oss << L"Fail to instansiate module class (" << m_unique_id << L"). check RegisterPlugin() matches XML file module id";
		SafeMessagebox(0, oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);
#else
		// ok	assert( false );
#endif
	}

	// STEP 7: Unload DLL
	// r = MP_DllUnload( &dll_handle );
	return 0;
}

ug_base* Module_Info3::BuildSynthOb()
{
	if( LoadDllOnDemand() || !m_dsp_registered )
	{
		return 0;
	}
#if 0
	gmpi::MP_DllEntry dll_entry_point;
int32_t r;
#ifdef _WIN32
	const char* gmpi_dll_entrypoint_name = "MP_GetFactory";
	r = MP_DllSymbol( dllHandle, gmpi_dll_entrypoint_name, (void**) &dll_entry_point );
#else
    dll_entry_point = NULL;
    dll_entry_point = (gmpi::MP_DllEntry) CFBundleGetFunctionPointerForName((CFBundleRef) dllHandle, CFSTR("MP_GetFactory"));
    if(dll_entry_point == 0) {
		printf("Couldn't get a pointer to plugin's main()\n");
//		CFBundleUnloadExecutable(dllHandle);
//		CFRelease(dllHandle);
		return 0;
        }
#endif        
 
	gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> factoryBase;
	r = dll_entry_point(factoryBase.asIMpUnknownPtr());
#endif
	auto factoryBase = getFactory2();

	int32_t r{};
	// Obtain factory V2 if avail.
	{
		// two step init. create then plugin then host.

		gmpi_sdk::mp_shared_ptr<gmpi::IMpFactory2> factory2;
		r = factoryBase->queryInterface(gmpi::MP_IID_FACTORY2, factory2.asIMpUnknownPtr());

		if (factory2)
		{
			// SE SDK3
			// Ask factory to instantiate plugin
			gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> plugin;
			r = factory2->createInstance2(m_unique_id.c_str(), gmpi::MP_SUB_TYPE_AUDIO, plugin.asIMpUnknownPtr());
			if (!plugin || r != gmpi::MP_OK)
			{
				return {};
			}

			{
				gmpi_sdk::mp_shared_ptr<gmpi::IMpPlugin2> my_plugin2;
				r = plugin->queryInterface(gmpi::MP_IID_PLUGIN2, my_plugin2.asIMpUnknownPtr());

				if (my_plugin2)
				{
					// Now plugin safely created. Host it.
					auto ug = new ug_plugin3<gmpi::IMpPlugin2, gmpi::MpEvent>();
					ug->AttachGmpiPlugin(my_plugin2);
					my_plugin2->setHost(static_cast<gmpi::IGmpiHost*>(ug));

					ug->setModuleType(this);
					ug->flags = static_cast<UgFlags>(ug_flags);
					ug->latencySamples = latency;

					return ug;
				}
			}
		}
		else
		{
			// GMPI SDK
			// only difference is utf8 ID instead of utf16
			gmpi_sdk::mp_shared_ptr<gmpi::api::IPluginFactory> factory;
			r = factoryBase->queryInterface((const gmpi::MpGuid&) gmpi::api::IPluginFactory::guid, factory.asIMpUnknownPtr());

			if (!factory || r != gmpi::MP_OK)
			{
				return {};
			}
			auto uniqueIdUtf8 = WStringToUtf8(m_unique_id);

			gmpi::shared_ptr<gmpi::api::IUnknown> plugin;
			r = (int32_t) factory->createInstance(uniqueIdUtf8.c_str(), gmpi::api::PluginSubtype::Audio, plugin.asIMpUnknownPtr());
			if (!plugin || r != gmpi::MP_OK)
			{
				return {};
			}

			{
				auto gmpi_plugin = plugin.As<gmpi::api::IAudioPlugin>();
				if (gmpi_plugin)
				{
					// Now plugin safely created. Host it.
					auto ug = new ug_plugin3<gmpi::api::IAudioPlugin, gmpi::api::Event>();
					ug->AttachGmpiPlugin(gmpi_plugin);
					ug->setModuleType(this);
					ug->flags = static_cast<UgFlags>(ug_flags);
					ug->latencySamples = latency;

					return ug;
				}
			}
		}

		{
			// Obtain factory.
			gmpi_sdk::mp_shared_ptr<gmpi::IMpFactory> factory;
			{
				r = factoryBase->queryInterface(gmpi::MP_IID_FACTORY, factory.asIMpUnknownPtr());
			}

			auto ug = new ug_plugin3<gmpi::IMpPlugin, gmpi::MpEvent>();
			gmpi::IMpHost* host = static_cast<gmpi::IMpHost*>( ug );

			// STEP 4: Ask factory to instantiate plugin
			gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> com_object2; // plugin.
			r = factory->createInstance(( m_unique_id ).c_str(), gmpi::MP_SUB_TYPE_AUDIO, ( gmpi::IMpUnknown* ) host, com_object2.asIMpUnknownPtr());

			//				ERROR_CHECK(r,"fail to create plugin\n");
			if( r == gmpi::MP_OK )
			{
				// STEP 5: Ask plugin to provide GIMPI Plugin V1 interface
				gmpi_sdk::mp_shared_ptr<gmpi::IMpPlugin> my_plugin;
				r = com_object2->queryInterface(gmpi::MP_IID_PLUGIN, my_plugin.asIMpUnknownPtr());
				//				ERROR_CHECK(r,"plugin does not support GMPI V 1.0 Interface\n");

				ug->AttachGmpiPlugin(my_plugin);

				ug->setModuleType(this);
				ug->latencySamples = latency;
				ug->flags = static_cast<UgFlags>( ug_flags );

				return ug;
			}
			else
			{
				// !! NOTE this will crash SE as downstream modules won't have anything to connect to.
				// current code uses HasActiveConnections() to avoid this, might be better to check each plug on each ug, deal to any with no connection (after all other connections made)

				std::wostringstream oss;
				oss << L"Fail to instansiate module audio class (" << m_unique_id << L"). check RegisterPlugin() matches XML file module id";
#if defined( SE_EDIT_SUPPORT )
				SafeMessagebox(0, oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);
#else
				assert(false);
#endif

				delete ug;
				ug = 0;
			}
		}
	}

	// STEP 7: Unload DLL
	//	r = MP_DllUnload( &dll_handle );

	return 0;
}

#if defined( SE_EDIT_SUPPORT )
std::wstring Module_Info3::showSdkVersion()
{
	int32_t sdkVersion = -1;
	const int32_t nCharacters = 100;
	wchar_t compilerInfo[nCharacters] = L"";

	if( !LoadDllOnDemand() )
	{
		const char* gmpi_dll_entrypoint_name = "MP_GetFactory";
		gmpi::MP_DllEntry MP_GetFactory;
		int32_t r = MP_DllSymbol( dllHandle, gmpi_dll_entrypoint_name, (void**) &MP_GetFactory );

		gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> com_object;
		r = MP_GetFactory(com_object.asIMpUnknownPtr());

		gmpi_sdk::mp_shared_ptr<gmpi::IMpFactory> factory;
		r = com_object->queryInterface(gmpi::MP_IID_FACTORY, factory.asIMpUnknownPtr());

		if (factory && r == MP_OK)
		{
			return L"No information about SDK available";
	}
	}

	std::wostringstream oss;
	if (sdkVersion > -1)
	{
	oss << L"SDK V" << ((float)sdkVersion * 0.0001f) << L", " << compilerInfo << L"\n";
	}
	oss << L"File: " << Filename();

	return oss.str();
}
#endif
