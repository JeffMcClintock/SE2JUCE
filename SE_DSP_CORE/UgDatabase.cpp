// Virtual base class for all unit generators
// (Anything that creates or modifies sound)
#include "pch.h"
#include <assert.h>
// Fix for <sstream> on Mac (sstream uses undefined int_64t)
#include "./modules/se_sdk3/mp_api.h"
#include "./modules/shared/xp_dynamic_linking.h"
#include <sstream>
#include "UgDatabase.h"
#include "ug_base.h"
#include "./modules/se_sdk3/MpString.h"
#include "./modules/tinyXml2/tinyxml2.h"
#include "conversion.h"
#include "RawConversions.h"

using namespace gmpi_sdk;

#if defined( SE_EDIT_SUPPORT )
#include "../SynthEdit/SynthEditdoc.h"
#include "ModuleFactory_Editor.h"
#include "../SynthEdit/international.h"
#include "../SynthEdit/SafeMessageBox.h"
#endif

#include "Module_Info3_internal.h"
#include "IPluginGui.h"
#include "tinyxml/tinyxml.h"
#include "version.h"
#include "HostControls.h"
#include "midi_defs.h"
#include "./modules/shared/xplatform.h"
#include "./modules/shared/xp_dynamic_linking.h"
#include "BundleInfo.h"

#if SE_EXTERNAL_SEM_SUPPORT==1
	#include "Module_Info3.h"
#endif

// provide extensibility to add extra modules on a per-project basis.
// SE2JUCE Controller must implement this function
extern void initialise_synthedit_extra_modules(bool passFalse);

typedef std::pair <std::wstring, Module_Info*> string_mod_info_pair;

using namespace std;
using namespace gmpi_dynamic_linking;


IMPLEMENT_SERIAL( Module_Info, CObject, 1 )

CModuleFactory::CModuleFactory()
{
	initialise_synthedit_modules();
}

gmpi::IMpUnknown* CModuleFactory::Create( int32_t subType, const std::wstring& uniqueId )
{
	Module_Info* mi = GetById( uniqueId );
	if( mi )
	{
		return mi->Build( subType );
	}

	return 0;
}

// when loading projects with non-available modules, a fake module info is created, but don't want it on Insert menu.
bool Module_Info::isDllAvailable()
{
	return m_module_dll_available;
}

bool Module_Info::getSerialiseFlag()
{
	return m_serialise_me || ((flags & CF_ALWAYS_EXPORT) != 0);
}

bool Module_Info::hasDspModule()
{
	// Note: SDK2 has DSP regardless of needed or not.
	if( ModuleTechnology() < MT_SDK3 )
	{
		// assume it has DSP if it has DSP pins.
		for( module_info_pins_t::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
		{
			if( (*it).second->isDualUiPlug(0) || !(*it).second->isUiPlug(0) )
				return true;
		}
		return false;
	}
	return m_dsp_registered;
}

#if defined( SE_EDIT_SUPPORT )
bool Module_Info::hasGuiModule()
{
	if (ModuleTechnology() < MT_SDK3)
	{
		// assume it has GUI if it has GUI pins.
		for (module_info_pins_t::iterator it = plugs.begin(); it != plugs.end(); ++it)
		{
			if ((*it).second->isDualUiPlug(0) || (*it).second->isUiPlug(0))
				return true;
		}
		return false;
	}
	return m_gui_registered;
}
#endif

bool Module_Info::OnDemandLoad()
{
	return true;
}

bool Module_Info::gui_object_non_visible()
{
	// CF_NON_VISIBLE used to include MP_WINDOW_TYPE_COMPOSITED, MP_WINDOW_TYPE_HWND, MP_WINDOW_TYPE_VSTGUI
	// but these are deprecated anyhow
	return getWindowType() != MP_WINDOW_TYPE_XP;
//	return ( GetFlags() & CF_NON_VISIBLE) != 0;
}

Module_Info* CModuleFactory::FindOrCreateModuleInfo( const std::wstring& p_unique_id  )
{
	map<std::wstring, Module_Info*>::iterator it = module_list.find( p_unique_id );

	if( it == module_list.end( ) )
	{
		Module_Info* mi = new Module_Info(p_unique_id);
		auto res = module_list.insert( string_mod_info_pair( (mi->UniqueId()), mi) );
		assert( res.second ); // insert failed.
		return mi;
	}
	else
	{
		return (*it).second;
	}
}

// register one or more *internal* plugins with the factory
int32_t RegisterPluginXml( const char* xmlFile )
{
	// waves puts all XML in project file. exception is "helper" modules not explicity in patch.
	ModuleFactory()->RegisterPluginsXml( xmlFile );
	return gmpi::MP_OK;
}

// New generic object register.
int32_t RegisterPlugin( int subType, const wchar_t* uniqueId, MP_CreateFunc2 create )
{
	return ModuleFactory()->RegisterPlugin( subType, uniqueId, create );
}

int32_t RegisterPluginWithXml(int subType, const char* xml, MP_CreateFunc2 create)
{
	return ModuleFactory()->RegisterPluginWithXml(subType, xml, create);
}

// register an internal ug_base derived module.
namespace internalSdk
{
	bool RegisterPlugin(ug_base *(*ug_create)(), const char* xml)
	{
		return ModuleFactory()->RegisterModule( new Module_Info(ug_create, xml) );
	}
}

int32_t CModuleFactory::RegisterPlugin( int subType, const wchar_t* uniqueId, MP_CreateFunc2 create )
{
	Module_Info3_internal* mi3 = 0;
	Module_Info* mi = GetById( uniqueId );

	if( mi )
	{
		mi3 = dynamic_cast<Module_Info3_internal*>( mi );

		if( mi3 == 0 )
		{
			assert( false && "wrong type of module" );
			return gmpi::MP_FAIL;
		}
	}
	else
	{
		mi3 = new Module_Info3_internal( uniqueId );
		auto res = module_list.insert( string_mod_info_pair( ( mi3->UniqueId() ), mi3 ) );
		assert( res.second ); // insert failed.
	}

	mi3->RegisterPluginConstructor( subType, create );

	return gmpi::MP_OK;
}

int32_t CModuleFactory::RegisterPluginWithXml(int subType, const char* xml, MP_CreateFunc2 create)
{
	auto mi3 = new Module_Info3_internal(xml);

	auto res = module_list.insert(string_mod_info_pair((mi3->UniqueId()), mi3));
	assert(res.second); // insert failed.
	mi3->RegisterPluginConstructor(subType, create);

	return gmpi::MP_OK;
}

// Register DSP section of module
// this may look unnesc, but results in smaller registration code per-module (module don't need to create new Module_Info() )
bool CModuleFactory::RegisterModule( module_description_internal& p_module_desc ) // obsolete
{
	//_RPT1(_CRT_WARN, "RegisterModule %s\n", p_module_desc.unique_id );
	FindOrCreateModuleInfo(Utf8ToWstring(p_module_desc.unique_id) )->Register(p_module_desc);
	return true;
}

// Register GUI section of module. SDK3 compatible
bool CModuleFactory::RegisterModule( get_module_properties_func mod_func, get_pin_properties_func pin_func)
{
	module_description mod;
	memset( &mod,0,sizeof(mod));
	mod_func(mod);
	//_RPT1(_CRT_WARN, "RegisterModule %s\n", mod.unique_id );
	FindOrCreateModuleInfo(Utf8ToWstring(mod.unique_id) )->Register(mod, pin_func);
	return true;
}

// REGISTER_MODULE_INTERNAL macro
void Module_Info::Register( struct module_description& mod, get_pin_properties_func get_pin_properties) // GUI
{
	flags			|= mod.flags;
	m_unique_id		= Utf8ToWstring(mod.unique_id);
	// split name into group/name
	std::wstring s(mod.name);
	size_t separator = s.find_last_of( L'/' );

	if( separator != string::npos ) // split name/groupname
	{
		m_group_name = s.substr(0, separator );
		m_name = s.substr(separator + 1, s.size() - separator - 1 );
	}
	else
	{
		m_name = s;
	}

	assert( !m_gui_registered );

	int i = 0;
	pin_description pin;
	bool more;

	do
	{
		const wchar_t* emptyString = L"";
		memset( &pin,0,sizeof(pin));
		pin.default_value = pin.meta_data = pin.name = pin.notes = emptyString;
		more = get_pin_properties( i, pin );

		if( more )
		{
			InterfaceObject* iob = new InterfaceObject( i, pin );
			iob->SetFlags( iob->GetFlags() | IO_UI_COMMUNICATION ); // set as GUI-Plug (saves every module adding this flag to every pin)
			gui_plugs.insert( std::pair<int,InterfaceObject*>(i,iob) );
			++i;
		}
	}
	while(more);

	m_gui_registered = true;
}

// Register DSP section of module
bool CModuleFactory::RegisterModule(module_description_dsp& p_module_desc)
{
	FindOrCreateModuleInfo(Utf8ToWstring(p_module_desc.unique_id) )->Register(p_module_desc);
	return true;
}

// register a module (old SDK2 style GUI pins).
// would probly make more sense to have GUI and DSP objects as two things, not two sections of one thing
void Module_Info::Register(module_description_internal& p_module_desc)
{
	flags |= p_module_desc.flags;
	m_unique_id = Utf8ToWstring( p_module_desc.unique_id );
	dsp_create = p_module_desc.ug_create;

	if( p_module_desc.name_string_resource_id > -1 )
	{
		sid_name = p_module_desc.name_string_resource_id;
		sid_group = p_module_desc.category_string_resource_id;
	}

	m_loaded_into_database = true;

	// new way, dosen't require instansiating, replaces SetupPlugs()
	for( int i = 0 ; i < p_module_desc.pin_descriptions_count ; i++ )
	{
		InterfaceObject* io = new InterfaceObject( i, p_module_desc.pin_descriptions[i] );
		plugs.insert( std::pair<int,InterfaceObject*>(i, io) );

		// detect old-style GUI pins
		if( 0 != (p_module_desc.pin_descriptions[i].flags & IO_UI_COMMUNICATION) )
		{
			io->SetFlags( io->GetFlags() | IO_OLD_STYLE_GUI_PLUG );
		}

		if( io->isParameterPlug(0) )
		{
			assert( io->getParameterId(0) == -1 );
			io->setParameterId(0); // assuming my own modules use only one parameter max.

			if( io->getParameterFieldId(0) == FT_VALUE )
			{
				RegisterSdk2Parameter( io );
			}
		}
	}

	m_dsp_registered = true;
}

// new way
void Module_Info::Register(module_description_dsp& p_module_desc)
{
	assert( !m_dsp_registered );
	flags |= p_module_desc.flags | CF_STRUCTURE_VIEW; // structure view assumed
	m_unique_id = Utf8ToWstring( p_module_desc.unique_id );
	dsp_create = p_module_desc.ug_create;

	if( p_module_desc.name_string_resource_id > -1 )
	{
		sid_name = p_module_desc.name_string_resource_id;
		sid_group = p_module_desc.category_string_resource_id;
	}

	m_loaded_into_database = true;

	// new way, dosen't require instansiating, replaces SetupPlugs()
	for( int i = 0 ; i < p_module_desc.pin_descriptions_count ; i++ )
	{
		InterfaceObject* io = new InterfaceObject( i, p_module_desc.pin_descriptions[i] );
		plugs.insert( std::pair<int,InterfaceObject*>(i, io) );

		if( (io->GetFlags() & IO_PATCH_STORE ) != 0 )
		{
			assert( io->getParameterId(0) == -1 );
			io->setParameterId(0); // assuming my own modules use only one parameter max.
			RegisterSdk2Parameter( io );
		}
	}

	m_dsp_registered = true;
}

// using older macros - REGISTER_MODULE_1_BC
bool CModuleFactory::RegisterModule(Module_Info* p_module_info)
{
	// ignore obsolete ones
	if( p_module_info->UniqueId().empty() )
	{
		delete p_module_info;
		return false;
	}

	auto res = module_list.insert( string_mod_info_pair( (p_module_info->UniqueId()), p_module_info) );

	/* no too early?
	#ifdef _DEBUG // check resource string exists
	p_module_info->GetName();
	p_module_info->GetGroupName();
	#endif
	*/
	//   {
	//	   p_module_info->Setup Plugs();
	// }
	//   else
	if( res.second == false )
	{
		if( !p_module_info->UniqueId().empty() )
		{
#if defined( SE_EDIT_SUPPORT )
			std::wostringstream oss;
			oss << L"RegisterModule Error. Module found twice. " << p_module_info->UniqueId();
			SafeMessagebox(0, oss.str().c_str(), L"", MB_OK|MB_ICONSTOP );
#endif
			assert(false);
		}

		delete p_module_info;
	}

	return true;
}

void Module_Info::setShellPlugin()
{
	flags |= CF_SHELL_PLUGIN;
}

bool Module_Info::isShellPlugin()
{
	return ( flags & CF_SHELL_PLUGIN ) != 0;
}

#if defined( SE_EDIT_SUPPORT )

void CModuleFactory::RegisterExternalPluginsXml(TiXmlDocument* doc, const std::wstring& full_path, const std::wstring& group_name, bool isShellPlugin)
{
	auto pluginList = doc->FirstChild( "PluginList" );

	if (pluginList == 0)
	{
		return;
	}
	bool reportedDuplicateModule = false;

	// Walk all plugins.
	for( auto  node = pluginList->FirstChild( "Plugin" ); node; node = node->NextSibling( "Plugin" ) )
	{
		// check for existing
		TiXmlElement* PluginElement = node->ToElement();
		wstring plugin_id = Utf8ToWstring( PluginElement->Attribute("id") );

		if( GetById(plugin_id) != nullptr )
		{
			auto sdk3Module = dynamic_cast<Module_Info3_base*>( GetById(plugin_id) );
			// Can't directly replace a internal modules with a external module using same ID. Need to write replacement code
			if( sdk3Module != 0 )
			{
				#if defined( SE_EDIT_SUPPORT )
				if( !reportedDuplicateModule )
				{
					wstring plugin_name = Utf8ToWstring(PluginElement->Attribute("name"));
					reportedDuplicateModule = true;
					std::wostringstream oss;
					oss << L"Module FOUND TWICE!, only one loaded. Please remove the oldest.  Name='" <<
						plugin_name << L"' id='" << plugin_id << L"'\n" <<
						L"\n" << full_path << L"\n" << sdk3Module->Filename();
					SafeMessagebox(0,oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);
				}
				#else
					assert(false);
				#endif
			}
			else
			{
				// Joystick Image etc have modern counterparts that are active on on 64-bit.
				// This prevents disruption to 32-bit users.
				if (plugin_id == L"ImageJoystick")
				{
					return;
				}
				assert( false && "Duplicate Module ID, but is not SDK3");
			}
		}
		else
		{
			Module_Info3* mi3 = new Module_Info3( full_path, group_name );
			mi3->ScanXml( PluginElement);
			if (isShellPlugin)
				mi3->setShellPlugin();

			auto res = module_list.insert(string_mod_info_pair((mi3->UniqueId()), mi3));
			assert( res.second ); // insert failed.
			mi3->setLoadedIntoDatabase();
		}
	}
}
#endif

void CModuleFactory::RegisterPluginsXml( const char* xml_data )
{
	TiXmlDocument doc;
	doc.Parse( xml_data );

	if ( doc.Error() )
	{
#if defined( SE_EDIT_SUPPORT )
		std::wostringstream oss;
		oss << L"Module XML Error: [SynthEdit.exe]" << doc.ErrorDesc() << L"." <<  doc.Value();
		SafeMessagebox(0, oss.str().c_str(), L"", MB_OK|MB_ICONSTOP );
#else
		for( int i = doc.ErrorCol() - 5 ; i < doc.ErrorCol() + 5 ; ++i )
		{
			_RPT1(_CRT_WARN, "%c", xml_data[i] );
		}
		_RPT0(_CRT_WARN, "\n");
		assert(false);
#endif

		return;
	}

	auto pluginList = doc.FirstChild("PluginList");

	if(pluginList) // Check it is a plugin description (not some other XML file in UAP)
		RegisterPluginsXml(pluginList);
}



#ifndef SE_EDIT_SUPPORT
void CModuleFactory::RegisterExternalPluginsXmlOnce(TiXmlNode* /* pluginList */)
{
	std::lock_guard<std::mutex> guard(RegisterExternalPluginsXmlOnce_lock);

	// In VST3, can be called from either Controller, or Processor (whichever the DAW initializes first)
	// ensure only init once.
	if (initializedFromXml)
	{
		return;
	}
	initializedFromXml = true;

	{
		// On VST3 database is separate XML to make loading faster.
		auto bundleinfo = BundleInfo::instance();

		// Modules database.
		auto databaseXml = bundleinfo->getResource("database.se.xml");
		TiXmlDocument doc;
		doc.Parse(databaseXml.c_str());

		if (doc.Error())
		{
			assert(false);
			return;
		}

		TiXmlHandle hDoc(&doc);
		auto document_xml = hDoc.FirstChildElement("Document").Element();
		RegisterPluginsXml(document_xml->FirstChildElement("PluginList"));
	}
}
#endif

void CModuleFactory::RegisterPluginsXml(TiXmlNode* pluginList )
{
	// Walk all plugins.
	for( auto node = pluginList->FirstChild( "Plugin" ); node; node = node->NextSibling( "Plugin" ) )
	{
		// check for existing
		TiXmlElement* PluginElement = node->ToElement();
		wstring pluginId = Utf8ToWstring( PluginElement->Attribute("id") );
		Module_Info3_base* mi3 = 0;
		Module_Info* mi = ModuleFactory()->GetById( pluginId );

		if( mi )
		{
			mi3 = dynamic_cast<Module_Info3_internal*>( mi );

			if( mi3 == 0 )
			{
				assert( false && "Duplicate Module" );
				return;
			}
			mi3->ScanXml( PluginElement);
		}
		else
		{
			wstring imbeddedFilename = Utf8ToWstring( PluginElement->Attribute("imbeddedFilename") );
			if( imbeddedFilename.empty() )
			{
				// Internal module compiled with engine.
				mi3 = new Module_Info3_internal( pluginId.c_str() );
			}
			else
			{
                #if SE_EXTERNAL_SEM_SUPPORT==1
				// External module in VST3 folder.
					mi3 = new Module_Info3( imbeddedFilename );
                #else
					// * Check module source file is included in project.
					// * Check source contains 'SE_DECLARE_INIT_STATIC_FILE' macro.
					// * Check this file contains 'INIT_STATIC_FILE' macro.
					// if JUCE, it's OK not to register GUI modules because they are not supported.
					_RPT1(0, "Module not available: %S\n", pluginId.c_str());
//                    assert(false);
					continue;
                #endif
			}

            mi3->ScanXml( PluginElement);

			auto res = module_list.insert( string_mod_info_pair( (mi3->UniqueId()), mi3) );
			assert( res.second ); // insert failed.
		}
	}
}

void CModuleFactory::ImportModuleInfo(tinyxml2::XMLElement* documentE, ExportFormatType fileType)
{
#if defined( SE_EDIT_SUPPORT )
	const bool loadingCache = (fileType == SAT_SEM_CACHE);

	// load module infos stored in file, add any unknown ones to database
	auto pluginList = documentE->FirstChildElement("PluginList");

	if (!pluginList)
		return;

	for (auto pluginE = pluginList->FirstChildElement(); pluginE; pluginE = pluginE->NextSiblingElement())
	{
		const auto plugin_id = Utf8ToWstring(pluginE->Attribute("id"));

		Module_Info* mi{};

		auto fileX = pluginE->Attribute("file");
		if (fileX)
		{
			mi = new Module_Info3(Utf8ToWstring(fileX));
		}
		else
		{
			mi = new Module_Info(plugin_id);
			mi->SetUnavailable(); // until creation function registered.
		}

		mi->Import(pluginE, fileType);

		// insert in module list, only if not already present
		auto res = module_list.insert(string_mod_info_pair((mi->UniqueId()), mi));

		if (res.second)
		{
			// insert OK, module not previously registered
			mi->setLoadedIntoDatabase();

			// when loading cache, not finding existing info is fine.
			if (!loadingCache)
			{
				CSynthEditDocBase::CantLoad(mi->UniqueId());
#if defined( _DEBUG )
				_RPTW1(_CRT_WARN, L"%s: Module info not avail, using stored info\n", mi->UniqueId().c_str());
#endif
			}
			else
			{
				//#if defined( _DEBUG )
				//	std::wstring t;
				//	t.Format( (L"[%s] Loaded"), mi->UniqueId() );
				//	_RPT1(_CRT_WARN, "%s\n", DebugCStringToAscii(t) );
				//#endif
			}
		}
		else
		{
			auto localModuleInfo = GetById(mi->UniqueId());

			// Check for wrapped VST2 plugins that are not available, only summary info.
			// In this case we prefer to use the full module information from the project file.
			if (!localModuleInfo->OnDemandLoad() && localModuleInfo->isSummary())
			{
				auto it = module_list.find(localModuleInfo->UniqueId());
				module_list.erase(it);
				delete localModuleInfo;

				auto res2 = module_list.insert(string_mod_info_pair((mi->UniqueId()), mi));

				assert(res2.second);
				{
					// insert OK, module not previously registered
					mi->setLoadedIntoDatabase();
				}
			}
			else
			{
				// store pointer so object can be deleted after load.
				{
					std::unique_ptr<Module_Info> ptr(mi);
					m_in_use_old_module_list.push_back(std::move(ptr));
				}

				if (!loadingCache)
				{
					// Check stored pin data compatible with local version of module (rough check). Ck_Waveshaper1 has this problem. two versions with same ID but different pin count.
					auto storedModuleInfo = mi;
					if (!isCompatibleWith(storedModuleInfo, localModuleInfo))
					{
						mi->m_incompatible_with_current_module = true;

						std::wostringstream oss;
						oss << L"WARNING This project was created with a different version of '" << localModuleInfo->GetName() << L"'.  This module will be replaced with your local version.";
						SafeMessagebox(0,oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);
					}
				}
			}
		}
	}
#endif
}


Module_Info::Module_Info(const std::wstring& p_unique_id) :
	flags(0)
	,m_unique_id(p_unique_id)
	,dsp_create(0)
	,sid_name(0)//-1)
	,sid_group(0)//-1)
	,m_loaded_into_database(true)
	,m_module_dll_available(true)
	, latency(0)
	, scanned_xml_dsp(false)
	, scanned_xml_gui(false)
	, scanned_xml_parameters(false)
{
	//_RPT0(_CRT_WARN, "create Module_Info\n" );
}

Module_Info::Module_Info(class ug_base *(*ug_create)(), const char* xml) :
	flags(0)
	, dsp_create(ug_create)
	, sid_name(0)
	, sid_group(0)
	, m_loaded_into_database(true)
	, m_module_dll_available(true)
	, latency(0)
	, scanned_xml_dsp(false)
	, scanned_xml_gui(false)
	, scanned_xml_parameters(false)
{
	TiXmlDocument doc2;
	doc2.Parse(xml);

	if (doc2.Error())
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
		auto pluginList = doc2.FirstChild("PluginList");
		auto node = pluginList->FirstChild("Plugin");
		assert(node);
		auto PluginElement = node->ToElement();
		m_unique_id = Utf8ToWstring(PluginElement->Attribute("id"));

		ScanXml(PluginElement);
	}
}

Module_Info::Module_Info() :
	m_loaded_into_database(false)
	,m_module_dll_available(true)
	,dsp_create(0)
	,sid_name(0)
	,sid_group(0)
	,flags(0)
	, latency(0)
	, scanned_xml_dsp(false)
	, scanned_xml_gui(false)
	, scanned_xml_parameters(false)
{}; // serialisation only

// REGISTER_MODULE_3_BC
Module_Info::Module_Info(const wchar_t* p_unique_id, int p_sid_name, int p_sid_group_name, CDocOb *( *cug_create )( Module_Info* ), ug_base *( *ug_create )( ), int p_flags) :
	flags(p_flags)
	,m_unique_id(p_unique_id)
	,dsp_create(ug_create)
	,sid_name(p_sid_name)
	,sid_group(p_sid_group_name)
	,m_loaded_into_database(true)
	,m_module_dll_available(true)
	, latency(0)
	, scanned_xml_dsp(false)
	, scanned_xml_gui(false)
	, scanned_xml_parameters(false)
{
	//_RPT0(_CRT_WARN, "create Module_Info\n" );
	flags |= CF_OLD_STYLE_LISTINTERFACE;
	/*testing
		if( p_type == 126 )
		{
			for(int i = 0; i < name.size() ; i++ )
			{
				wchar_t c = name[i];
				_RPT2(_CRT_WARN, "%c %d\n", c, (int)c );
			}
		}
		*/
	SetupPlugs();
}

Module_Info::Module_Info(const wchar_t* p_unique_id, const wchar_t* name, const wchar_t* group_name, CDocOb *( *cug_create )( Module_Info* ), ug_base *( *ug_create )( ), int p_flags) :
flags(p_flags)
, m_unique_id(p_unique_id)
, dsp_create(ug_create)
, sid_name(0)
, sid_group(0)
, m_name(name)
, m_group_name(group_name)
, m_loaded_into_database(true)
, m_module_dll_available(true)
, latency(0)
, scanned_xml_dsp(false)
, scanned_xml_gui(false)
, scanned_xml_parameters(false)
{
	flags |= CF_OLD_STYLE_LISTINTERFACE;

	SetupPlugs();
}

Module_Info::~Module_Info()
{
	//	_RPT0(_CRT_WARN, "delete ~Module_Info\n" );
	ClearPlugs();
}

std::wstring Module_Info::GetGroupName()
{
#if defined( SE_EDIT_SUPPORT )

	if( sid_group != 0 )
		return GETSTRING(sid_group);

#endif
	return ( m_group_name );
}

std::wstring Module_Info::GetHelpUrl()
{
	return helpUrl_;
}
#if defined( SE_EDIT_SUPPORT )
std::wstring Module_Info::showSdkVersion()
{
	if( ModuleTechnology() == MT_SDK2 )
	{
		//			m.Format( (L"SDK V2. ID: %s."), getType()->UniqueId() );
		return L"SDK V2";
	}
	else
	{
		//m.Format( (L"Internal SDK. ID: %s."), getType()->UniqueId() );
		return L"Internal SDK";
	}
}
#endif

std::wstring Module_Info::GetName()
{
#if defined( SE_EDIT_SUPPORT )

	if( sid_name != 0 )
	{
		wstring r = GETSTRING(sid_name);

		if( !r.empty() )
			return r;
	}

#endif
	return ( m_name );
}

void Module_Info::ClearPlugs()
{
	for( auto& it : plugs )
	{
		delete it.second;
	}

	for (auto& it : gui_plugs)
	{
		delete it.second;
	}

	for (auto& it : controller_plugs)
	{
		delete it.second;
	}

	for (auto& it : m_parameters)
	{
		delete it.second;
	}

	plugs.clear();
	gui_plugs.clear();
	controller_plugs.clear();
	m_parameters.clear();
}

ug_base* Module_Info::BuildSynthOb()
{
	if( dsp_create == NULL )
		return NULL; // trying to build an object that can't be (display-only object)

	ug_base* module = dsp_create();
	module->moduleType = this;
	module->latencySamples = latency;
	return module;
}

#if defined( SE_EDIT_SUPPORT )
void Module_Info::Serialize( CArchive& ar )
{
	CObject::Serialize( ar );

	if( ar.IsStoring() )
	{
		ar <<  m_unique_id;
		ar <<  sid_name;
		ar <<  sid_group;
		ar <<  flags;
		ar << m_name;
		ar << m_group_name;
		ar << helpUrl_;
		ar << latency;

		// Plugs

		// Since V 130030
		ar << (int32_t)controller_plugs.size();
		for (auto& it : controller_plugs)
		{
			ar << it.second;
		}

		int plug_count = (int) plugs.size();
		ar << plug_count;
		int i = 1;

		for( module_info_pins_t::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
		{
			ar << (*it).second;
			// only last one autoduplicate else XML will add it to end getting DSP index wrong.
			// I/O plugs are exception. They are sorted to end.
			assert( !(*it).second->autoDuplicate(0) || i == plug_count || m_name == L"Container" );
			++i;
		}

		plug_count = (int) gui_plugs.size();
		ar << plug_count;

		for (auto& it : gui_plugs)
		{
			ar << it.second;
		}

		ar << (int) m_parameters.size();

		for( auto& it : m_parameters )
		{
			auto param = it.second;
			ar << param->id;
			ar << param->datatype;
			ar << param->flags;
			ar << param->name;
			ar << param->metaData; // new Since V 130040
		}

		SetSerialiseFlag();
	}
	else
	{
		// module info loaded as part of an se1 project can not be used to load a module dll.
		if( CSynthEditDocBase::serializingMode == SERT_SE1_DOC )
		{
			m_module_dll_available = false;
		}

		ar >>  m_unique_id;
		ar >>  sid_name;
		ar >>  sid_group;
		ar >>  flags;

		int plug_count;
		InterfaceObject* tempPlug;

		// TODO: these are not needed for save-as-vst? what else?
		ar >> m_name;
		ar >> m_group_name;

		ar >> helpUrl_;

		ar >> latency;

		ar >> plug_count;

		for (int i = 0; i < plug_count; i++)
		{
			ar >> tempPlug;
			controller_plugs.insert(std::pair<int, InterfaceObject*>(tempPlug->getPlugDescID(0), tempPlug));
		}

		// Plugs
		ar >> plug_count;

		for( int i = 0 ; i < plug_count ; i++ )
		{
			ar >> tempPlug;
			plugs.insert( std::pair<int,InterfaceObject*>(tempPlug->getPlugDescID(0),tempPlug) );
		}

		ar >> plug_count;

		for( int i = 0 ; i < plug_count ; i++ )
		{
			ar >> tempPlug;
			gui_plugs.insert( std::pair<int,InterfaceObject*>(tempPlug->getPlugDescID(0),tempPlug) );
		}

		ar >> plug_count;

		for( int i = 0 ; i < plug_count ; i++ )
		{
			parameter_description* param = new parameter_description;
			ar >> param->id;

			int temp;
			ar >> temp;
			param->datatype = (EPlugDataType) temp;
			ar >> param->flags;
			ar >> param->name;
			ar >> param->metaData;
			bool res = m_parameters.insert( std::pair<int,parameter_description*>( param->id, param ) ).second;
			assert( res == true );
		}
	}
}

void Module_Info::SaveModuleInfoPinXml(InterfaceObject* pin, ExportFormatType format, TiXmlElement* DspXml, int& expectedId)
{
	TiXmlElement* pinXml = new TiXmlElement("Pin");
	DspXml->LinkEndChild(pinXml);

	if (expectedId != pin->getPlugDescID(0))
	{
		expectedId = pin->getPlugDescID(0);
		pinXml->SetAttribute("id", expectedId);
	}

	pinXml->SetAttribute("name", WStringToUtf8(pin->GetName()));
	pinXml->SetAttribute("datatype", XmlStringFromDatatype(pin->GetDatatype()));
	if (pin->GetDatatype() == DT_FSAMPLE)
	{
		pinXml->SetAttribute("rate", "audio");
	}

	char *direction = 0; // or "in" (default)
	if (pin->GetDirection() == DR_OUT)
	{
		direction = "out";
	}
	else
	{
		wstring default = pin->GetDefault(0);
		if (!default.empty())
		{
			bool needsDefault = true;
			// In SDK3 Audio data defaults are literal (not volts).
			if (pin->GetDatatype() == DT_FSAMPLE || pin->GetDatatype() == DT_FLOAT || pin->GetDatatype() == DT_DOUBLE)
			{
				float d = StringToFloat(default); // won't cope with large 64bit doubles.

				if( pin->GetDatatype() == DT_FSAMPLE )
				{
					d *= 0.1f; // Volts to actual value.
				}

				default = FloatToString(d);

				needsDefault = d != 0.0f;
			}
			if( pin->GetDatatype() == DT_ENUM || pin->GetDatatype() == DT_BOOL || pin->GetDatatype() == DT_INT || pin->GetDatatype() == DT_INT64 )
			{
				int d = StringToInt(default); // won't cope with large 64bit ints.
				default = IntToString(d);

				needsDefault = d != 0;
			}

			if( pin->GetDatatype() == DT_MIDI2 )
			{
				needsDefault = false;
			}

			if( needsDefault )
			{
				pinXml->SetAttribute("default", WStringToUtf8(default));
			}
		}
	}

	if (direction)
	{
		pinXml->SetAttribute("direction", direction);
	}

	if (pin->DisableIfNotConnected(0))
	{
		pinXml->SetAttribute("private", "true");
	}
	if (pin->isRenamable(0))
	{
		pinXml->SetAttribute("autoRename", "true");
	}
	if (pin->is_filename(0))
	{
		pinXml->SetAttribute("isFilename", "true");
	}
	if ((pin->GetFlags() & IO_LINEAR_INPUT) != 0)
	{
		pinXml->SetAttribute("linearInput", "true");
	}
	if ((pin->GetFlags() & IO_IGNORE_PATCH_CHANGE) != 0)
	{
		pinXml->SetAttribute("ignorePatchChange", "true");
	}
	if ((pin->GetFlags() & IO_AUTODUPLICATE) != 0)
	{
		pinXml->SetAttribute("autoDuplicate", "true");
	}
	//if ((pin->GetFlags() & IO_PARAMETER_SCREEN_ONLY) != 0)
	//{
	//	pinXml->SetAttribute("noAutomation", "true");
	//}
	if ((pin->GetFlags() & IO_MINIMISED) != 0 || (pin->GetFlags() & IO_PARAMETER_SCREEN_ONLY) != 0)
	{
		pinXml->SetAttribute("isMinimised", "true");
	}
	if ((pin->GetFlags() & IO_PAR_POLYPHONIC) != 0)
	{
		pinXml->SetAttribute("isPolyphonic", "true");
	}
	if ((pin->GetFlags() & IO_AUTOCONFIGURE_PARAMETER) != 0)
	{
		pinXml->SetAttribute("autoConfigureParameter", "true");
	}

	if (pin->getParameterId(0) != -1)
	{
		pinXml->SetAttribute("parameterId", (int) pin->getParameterId(0));

		if (pin->getParameterFieldId(0) != FT_VALUE)
		{
			if(format == SAT_VST3)
			{
				pinXml->SetAttribute("parameterField", pin->getParameterFieldId(0)); // plain int for speed in compiled plugin.
			}
			else
			{
				pinXml->SetAttribute("parameterField", XmlStringFromParameterField(pin->getParameterFieldId(0)));
			}
		}
	}

	HostControls hostControlId = pin->getHostConnect(0);
	if (hostControlId != HC_NONE)
	{
		pinXml->SetAttribute("hostConnect", WStringToUtf8(GetHostControlName(hostControlId)));
		if (pin->getParameterFieldId(0) != FT_VALUE)
		{
//			pinXml->SetAttribute("parameterField", XmlStringFromParameterField(pin->getParameterFieldId(0)));
			if (format == SAT_VST3)
			{
				pinXml->SetAttribute("parameterField", pin->getParameterFieldId(0)); // plain int for speed in compiled plugin.
			}
			else
			{
				pinXml->SetAttribute("parameterField", XmlStringFromParameterField(pin->getParameterFieldId(0)));
			}
		}
	}

	if (!pin->GetEnumList().empty())
	{
		pinXml->SetAttribute("metadata", WStringToUtf8(pin->GetEnumList()));
	}
}

void Module_Info::SaveModuleInfoPinXml(InterfaceObject* pin, ExportFormatType format, tinyxml2::XMLElement* DspXml, int& expectedId)
{
	auto  pinXml = DspXml->GetDocument()->NewElement("Pin");
	DspXml->LinkEndChild(pinXml);

	if (expectedId != pin->getPlugDescID(0))
	{
		expectedId = pin->getPlugDescID(0);
		pinXml->SetAttribute("id", expectedId);
	}

	pinXml->SetAttribute("name", WStringToUtf8(pin->GetName()).c_str());

	if (pin->GetDatatype() == DT_CLASS)
	{
		if (!pin->GetClassName({}).empty())
		{
			pinXml->SetAttribute("datatype", ("class:" + pin->GetClassName({})).c_str());
		}
		else
		{
			pinXml->SetAttribute("datatype", "class");
		}
	}
	else
	{
		pinXml->SetAttribute("datatype", XmlStringFromDatatype(pin->GetDatatype()).c_str());
		if (pin->GetDatatype() == DT_FSAMPLE)
		{
			pinXml->SetAttribute("rate", "audio");
		}
	}

	char *direction = 0; // or "in" (default)
	if (pin->GetDirection() == DR_OUT)
	{
		direction = "out";
	}
	else
	{
		wstring default = pin->GetDefault(0);
		if (!default.empty())
		{
			bool needsDefault = true;
			// In SDK3 Audio data defaults are literal (not volts).
			if (pin->GetDatatype() == DT_FSAMPLE || pin->GetDatatype() == DT_FLOAT || pin->GetDatatype() == DT_DOUBLE)
			{
				float d = StringToFloat(default); // won't cope with large 64bit doubles.

				if (pin->GetDatatype() == DT_FSAMPLE)
				{
					d *= 0.1f; // Volts to actual value.
				}

				default = FloatToString(d);

				needsDefault = d != 0.0f;
			}
			if (pin->GetDatatype() == DT_ENUM || pin->GetDatatype() == DT_BOOL || pin->GetDatatype() == DT_INT || pin->GetDatatype() == DT_INT64)
			{
				int d = StringToInt(default); // won't cope with large 64bit ints.
				default = IntToString(d);

				needsDefault = d != 0;
			}

			if (pin->GetDatatype() == DT_MIDI2)
			{
				needsDefault = false;
			}

			if (needsDefault)
			{
				pinXml->SetAttribute("default", WStringToUtf8(default).c_str());
			}
		}
	}

	if (direction)
	{
		pinXml->SetAttribute("direction", direction);
	}

	if (pin->DisableIfNotConnected(0))
	{
		pinXml->SetAttribute("private", "true");
	}
	if (pin->isRenamable(0))
	{
		pinXml->SetAttribute("autoRename", "true");
	}
	if (pin->is_filename(0))
	{
		pinXml->SetAttribute("isFilename", "true");
	}
	if ((pin->GetFlags() & IO_LINEAR_INPUT) != 0)
	{
		pinXml->SetAttribute("linearInput", "true");
	}
	if ((pin->GetFlags() & IO_IGNORE_PATCH_CHANGE) != 0)
	{
		pinXml->SetAttribute("ignorePatchChange", "true");
	}
	if ((pin->GetFlags() & IO_AUTODUPLICATE) != 0)
	{
		pinXml->SetAttribute("autoDuplicate", "true");
	}
	if ((pin->GetFlags() & IO_MINIMISED) != 0)
	{
		pinXml->SetAttribute("isMinimised", "true");
	}
	if ((pin->GetFlags() & IO_PAR_POLYPHONIC) != 0)
	{
		pinXml->SetAttribute("isPolyphonic", "true");
	}
	if ((pin->GetFlags() & IO_AUTOCONFIGURE_PARAMETER) != 0)
	{
		pinXml->SetAttribute("autoConfigureParameter", "true");
	}
	if ((pin->GetFlags() & IO_PARAMETER_SCREEN_ONLY) != 0)
	{
		pinXml->SetAttribute("noAutomation", "true");
	}

	if (pin->getParameterId(0) != -1)
	{
		pinXml->SetAttribute("parameterId", (int)pin->getParameterId(0));

		if (pin->getParameterFieldId(0) != FT_VALUE)
		{
			if (format == SAT_VST3)
			{
				pinXml->SetAttribute("parameterField", pin->getParameterFieldId(0)); // plain int for speed in compiled plugin.
			}
			else
			{
				pinXml->SetAttribute("parameterField", XmlStringFromParameterField(pin->getParameterFieldId(0)).c_str());
			}
		}
	}

	HostControls hostControlId = pin->getHostConnect(0);
	if (hostControlId != HC_NONE)
	{
		pinXml->SetAttribute("hostConnect", WStringToUtf8(GetHostControlName(hostControlId)).c_str());
		if (pin->getParameterFieldId(0) != FT_VALUE)
		{
			//			pinXml->SetAttribute("parameterField", XmlStringFromParameterField(pin->getParameterFieldId(0)));
			if (format == SAT_VST3)
			{
				pinXml->SetAttribute("parameterField", pin->getParameterFieldId(0)); // plain int for speed in compiled plugin.
			}
			else
			{
				pinXml->SetAttribute("parameterField", XmlStringFromParameterField(pin->getParameterFieldId(0)).c_str());
			}
		}
	}

	if (!pin->GetEnumList().empty())
	{
		pinXml->SetAttribute("metadata", WStringToUtf8(pin->GetEnumList()).c_str());
	}
}

tinyxml2::XMLElement* Module_Info::Export(tinyxml2::XMLElement* element, ExportFormatType format, const string& overrideModuleId, const std::string& overrideModuleName)
{
	auto doc = element->GetDocument();

	auto pluginXml = doc->NewElement("Plugin");
	element->LinkEndChild(pluginXml);

	string uniqueId = WStringToUtf8(m_unique_id);
	if (!overrideModuleId.empty())
	{
		uniqueId = overrideModuleId;
	}

	string moduleName = WStringToUtf8(GetName());

	if (!overrideModuleName.empty())
	{
		moduleName = overrideModuleName;
	}

	pluginXml->SetAttribute("id", uniqueId.c_str());
	pluginXml->SetAttribute("name", moduleName.c_str());

	if (format != SAT_VST3 && format != SAT_CODE_SKELETON)
	{
		pluginXml->SetAttribute("flags", flags);
	}

	if (isShellPlugin())
	{
		pluginXml->SetAttribute("shellPlugin", true);
	}

	if ((flags & CF_ALWAYS_EXPORT) != 0 && format == SAT_CODE_SKELETON)
	{
		pluginXml->SetAttribute("alwaysExport", true);
	}

	if (format != SAT_VST3)
	{
		pluginXml->SetAttribute("category", WStringToUtf8(GetGroupName()).c_str());
		if (!GetHelpUrl().empty())
		{
			string helpfile = WStringToUtf8(GetHelpUrl());
			if (!overrideModuleId.empty())
			{
				helpfile = overrideModuleId + ".html";
			}
			pluginXml->SetAttribute("helpUrl", helpfile.c_str());
		}
	}
	// pluginXml->SetAttribute( "vendor", WStringToUtf8( ?? ));

	// Just for modules imbedded with VST3 plugins.
	Module_Info3_base* mi3 = dynamic_cast<Module_Info3_base*>(this);
	if (mi3 && format == SAT_VST3)
	{
		pluginXml->SetAttribute("imbeddedFilename", WStringToUtf8(StripPath(mi3->Filename())).c_str());
	}

	// Parameters.
	if (!m_parameters.empty())
	{
		auto parametersXml = doc->NewElement("Parameters");
		pluginXml->LinkEndChild(parametersXml);

		for (module_info_parameter_t::iterator it = m_parameters.begin(); it != m_parameters.end(); ++it)
		{
			parameter_description* param = (*it).second;

			auto parameterXml = doc->NewElement("Parameter");
			parametersXml->LinkEndChild(parameterXml);
			parameterXml->SetAttribute("id", param->id);
			parameterXml->SetAttribute("datatype", XmlStringFromDatatype((int)param->datatype).c_str());
			parameterXml->SetAttribute("name", WStringToUtf8(param->name).c_str());
			if (!param->metaData.empty())
				parameterXml->SetAttribute("metadata", WStringToUtf8(param->metaData).c_str());
			if (!param->defaultValue.empty())
				parameterXml->SetAttribute("default", WStringToUtf8(param->defaultValue).c_str());

			int controllerType = param->automation >> 24;
			if (controllerType != ControllerType::None)
			{
				parameterXml->SetAttribute("automation", XmlStringFromController(controllerType).c_str());
			}
			//ar << param->flags;
			if ((param->flags & IO_PAR_PRIVATE) != 0)
			{
				parameterXml->SetAttribute("private", "true");
			}
			if ((param->flags & IO_IGNORE_PATCH_CHANGE) != 0)
			{
				parameterXml->SetAttribute("ignorePatchChange", "true");
			}
			if ((param->flags & IO_FILENAME) != 0)
			{
				parameterXml->SetAttribute("isFilename", "true");
			}
			if ((param->flags & IO_PAR_POLYPHONIC) != 0)
			{
				parameterXml->SetAttribute("isPolyphonic", "true");
			}
			if ((param->flags & IO_PAR_POLYPHONIC_GATE) != 0)
			{
				parameterXml->SetAttribute("isPolyphonicGate", "true");
			}
			if ((param->flags & IO_PARAMETER_PERSISTANT) == 0)
			{
				parameterXml->SetAttribute("persistant", "false");
			}
		}
	}

	bool hasOldGuiPlugs = false;
	bool hasDspPlugs = false;
	for (auto it = plugs.begin(); it != plugs.end(); ++it)
	{
		auto pin = (*it).second;
		if (pin->isUiPlug(0))
		{
			hasOldGuiPlugs = true;
		}

		if (!pin->isUiPlug(0) || pin->isDualUiPlug(0))
		{
			hasDspPlugs = true;
		}
	}

	// DSP plugs.
	if (hasDspPlugs)
	{
		auto DspXml = doc->NewElement("Audio");
		pluginXml->LinkEndChild(DspXml);
		int id = 0;
		for (auto it = plugs.begin(); it != plugs.end(); ++it)
		{
			auto pin = (*it).second;

			if (!pin->isUiPlug(0) || pin->isDualUiPlug(0))
			{
				SaveModuleInfoPinXml(pin, format, DspXml, id);
				++id;
			}
		}
	}

	{
		// GUI plugs.
		tinyxml2::XMLElement* DspXml = 0;
		if (scanned_xml_gui || hasOldGuiPlugs || !gui_plugs.empty())
		{
			DspXml = doc->NewElement("GUI");
			pluginXml->LinkEndChild(DspXml);

			// graphicsApi.
			char* graphicsApi = 0;
			switch (getWindowType())
			{
			case MP_WINDOW_TYPE_WPF:
				graphicsApi = "WPF";
				break;
			case MP_WINDOW_TYPE_WPF_INTERNAL:
				graphicsApi = "WPF-internal";
				break;
			case MP_WINDOW_TYPE_COMPOSITED:
				graphicsApi = "composited";
				break;
			case MP_WINDOW_TYPE_HWND:
				graphicsApi = "HWND";
				break;
			case MP_WINDOW_TYPE_XP:
				graphicsApi = "GmpiGui";
				break;
			default:
				graphicsApi = 0; // or "none";
			}

			if (graphicsApi)
			{
				if (format == SAT_CODE_SKELETON)
					graphicsApi = "GmpiGui";

				DspXml->SetAttribute("graphicsApi", graphicsApi);
			}

			// Default for these is 'true', so only set if false.
			if ((flags & CF_PANEL_VIEW) == 0 && !hasOldGuiPlugs) // SDK2 GUI modules only spawn on structure. However when upgrading to SDK3 they need to appear on GUI so ignore this flag.
			{
				DspXml->SetAttribute("DisplayOnPanel", "false");
			}
			if ((flags & CF_STRUCTURE_VIEW) == 0)
			{
				DspXml->SetAttribute("DisplayOnStructure", "false");
			}
		}

		if (hasOldGuiPlugs)
		{
			int id = 0;
			for (auto it = plugs.begin(); it != plugs.end(); ++it)
			{
				auto pin = (*it).second;
				if (pin->isUiPlug(0))
				{
					SaveModuleInfoPinXml(pin, format, DspXml, id);
					++id;
				}
			}
		}

		//		if (!gui_plugs.empty())
		{
			int id = 0;
			for (auto it = gui_plugs.begin(); it != gui_plugs.end(); ++it)
			{
				auto pin = (*it).second;

				SaveModuleInfoPinXml(pin, format, DspXml, id);
				++id;
			}
		}
	}

	// Controller pins
	if (!controller_plugs.empty())
	{
		auto DspXml = doc->NewElement("Controller");
		pluginXml->LinkEndChild(DspXml);
		int id = 0;
		for (auto& it : controller_plugs)
		{
			auto pin = it.second;

			SaveModuleInfoPinXml(pin, format, DspXml, id);
			++id;
		}
	}

	return pluginXml;
}

// forward declare function from Module_Info3_base.cpp
void SetPinFlag(const char* xml_name, int xml_flag, tinyxml2::XMLElement* pin, int& flags);

void Module_Info::Import(tinyxml2::XMLElement* pluginE, ExportFormatType format)
{
	m_unique_id = Utf8ToWstring(pluginE->Attribute("id"));

	if (m_unique_id.empty())
	{
		m_unique_id = L"<!!!BLANK-ID!!!>"; // error
	}

	m_name = Utf8ToWstring(pluginE->Attribute("name"));

	if (m_name.empty())
	{
		m_name = m_unique_id; // L"<UN-NAMED>";
	}

	m_group_name = Utf8ToWstring(pluginE->Attribute("category"));
	helpUrl_ = Utf8ToWstring(pluginE->Attribute("helpUrl"));
#if defined( SE_EDIT_SUPPORT )
	std::string oldWindowType = FixNullCharPtr(pluginE->Attribute("windowType"));

	if (!oldWindowType.empty())
	{
		SafeMessagebox(0, L"XML: 'windowType' obsolete. Use 'graphicsApi'", L"", MB_OK | MB_ICONSTOP);
	}

#endif

	SetPinFlag("polyphonicSource", CF_NOTESOURCE | CF_DONT_EXPAND_CONTAINER, pluginE, flags);
	SetPinFlag("alwaysExport", CF_ALWAYS_EXPORT, pluginE, flags);
	SetPinFlag("shellPlugin", CF_SHELL_PLUGIN, pluginE, flags);

	// get parameter info.
	auto parameters = pluginE->FirstChildElement("Parameters");

	if (parameters)
	{
		RegisterParameters(parameters);
	}

	if (pluginE->FirstChildElement("Controller"))
	{
		// nothing to do, just note it's existance. todo, hassle for now.
		// m_controller_registered = true;
	}

	int scanTypes[] = { gmpi::MP_SUB_TYPE_AUDIO, gmpi::MP_SUB_TYPE_GUI, gmpi::MP_SUB_TYPE_CONTROLLER };
	for (auto sub_type : scanTypes)
	{
		string sub_type_name;

		switch (sub_type)
		{
		case gmpi::MP_SUB_TYPE_AUDIO:
			sub_type_name = "Audio";
			break;

		case gmpi::MP_SUB_TYPE_GUI:
			sub_type_name = "GUI";
			break;

		case gmpi::MP_SUB_TYPE_CONTROLLER:
			sub_type_name = "Controller";
			break;
		}

		auto plugin_class = pluginE->FirstChildElement(sub_type_name.c_str());

		if (plugin_class)
		{
			if ((sub_type == gmpi::MP_SUB_TYPE_AUDIO && scanned_xml_dsp) || (sub_type == gmpi::MP_SUB_TYPE_GUI && scanned_xml_gui)) // already
			{
#if defined( SE_EDIT_SUPPORT )
				std::wostringstream oss;
				oss << L"Module FOUND TWICE!, only one loaded\n" << Filename();
				SafeMessagebox(0, oss.str().c_str(), L"", MB_OK | MB_ICONSTOP);
#else
				assert(false);
#endif
			}
			else
			{
				if (sub_type == gmpi::MP_SUB_TYPE_AUDIO)
				{
					plugin_class->ToElement()->QueryIntAttribute("latency", &latency);
				}

				RegisterPins(plugin_class, sub_type);

				if (sub_type == gmpi::MP_SUB_TYPE_GUI && scanned_xml_gui)
				{
					// Override asssumptions made by RegisterPins() if need be.
//					TiXmlElement* guiElement = plugin_class->ToElement();
					const char* s = plugin_class->Attribute("DisplayOnPanel");

					if (s && strcmp(s, "false") == 0)
					{
						CLEAR_BITS(flags, CF_PANEL_VIEW);
					}
					s = plugin_class->Attribute("DisplayOnStructure");

					if (s && strcmp(s, "false") == 0)
					{
						CLEAR_BITS(flags, CF_STRUCTURE_VIEW);
					}
				}
			}
		}
	}

	// If GUI XML found, assume module can create a GUI class.
	if (scanned_xml_gui)
	{
		m_gui_registered = true;
	}

	// If DSP XML found, assume module can create a DSP class.
	if (scanned_xml_dsp)
	{
		m_dsp_registered = true;
		flags |= CF_STRUCTURE_VIEW;
	}

	auto patchPoints_xml = pluginE->FirstChildElement("PatchPoints");

	if (patchPoints_xml)
	{
		for (auto param = patchPoints_xml->FirstChildElement("PatchPoint"); param; param = param->NextSiblingElement("PatchPoint"))
		{
			auto pData2 = param->ToElement();
			//			auto pp = new patchpoint_description();

			patchPoints.push_back(patchpoint_description());

			auto& pp = patchPoints.back();

			// DSP Pin ID.
			pData2->QueryIntAttribute("pinId", &(pp.dspPin));

			// Center.
			pp.x = pp.y = 0;

			std::string centerPointS = FixNullCharPtr(pData2->Attribute("center"));
			size_t p = centerPointS.find_first_of(',');
			if (p != std::string::npos)
			{
				auto sx = centerPointS.substr(0, p);
				auto sy = centerPointS.substr(p + 1);

				pp.x = StringToInt(sx.c_str());
				pp.y = StringToInt(sy.c_str());
			}

			pData2->QueryIntAttribute("radius", &(pp.radius));
		}
	}


#if 0 // defined( _DEBUG ) && defined( SE_EDIT_SUPPORT )
	// Verify fidelity between original XML and save-as-vst XML re-generation.
	{
		TiXmlDocument doc;
		TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "", "");
		doc.LinkEndChild(decl);
		TiXmlElement* ModulesElement = new TiXmlElement("Plugins");
		doc.LinkEndChild(ModulesElement);
		ExportXml(ModulesElement);

		auto  node = ModulesElement->FirstChild("Plugin");
		TiXmlElement* PluginElement = node->ToElement();

		_RPTW1(_CRT_WARN, L"\nCOMPARE MODULE XML: %s \n", m_unique_id.c_str());
		auto  original = pluginE;
		auto  copy = PluginElement;

		CompareXml(original, copy);
	}

#endif
}
#endif


// Meyer's singleton
CModuleFactory* CModuleFactory::Instance()
{
	static CModuleFactory obj;
	return &obj;
}

CModuleFactory::~CModuleFactory()
{
	for( auto& it : module_list)
	{
		delete it.second;
	}

	module_list.clear();

	// Check for modules which we forgot to static init in UgDatabase.cpp
#if defined(_DEBUG) && defined(SE_TARGET_PLUGIN)

	for (auto& s : staticInitCheck2)
	{
		if (s.find("SE ") == 0 || s.find("SE:") == 0)
		{
			s = s.substr(3);
		}
		std::replace(s.begin(), s.end(), ' ', '_');
	}

	// delete even elements from the vector
	staticInitCheck2.erase(std::remove_if(staticInitCheck2.begin(), staticInitCheck2.end(), [this](const std::string& x) {
		return std::find(staticInitCheck.begin(), staticInitCheck.end(), x) != staticInitCheck.end();
		}), staticInitCheck2.end());

	for (const auto s : staticInitCheck2)
	{
		_RPTN(0, "INIT_STATIC_FILE(%s);\n", s.c_str());
	}
	assert(staticInitCheck2.empty()); // you have omitted a INIT_STATIC_FILE in UgDatabase.cpp for this module

#endif
}

void CModuleFactory::ClearModuleDb( const std::wstring& p_extension )
{
	for( auto it = PrefabFileNames.begin() ;  it != PrefabFileNames.end() ; )
	{
		std::wstring extension = GetExtension( *it );

		if( extension.compare( p_extension ) == 0 )
		{
			it = PrefabFileNames.erase(it);
		}
		else
		{
			++it;
		}
	}
}



// On save, relevant module infos are flagged for saving
void CModuleFactory::ClearSerialiseFlags()
{
	for( auto it = module_list.begin(); it != module_list.end(); ++it )
	{
		(*it).second->ClearSerialiseFlag();
	}
}

void CModuleFactory::SerialiseModuleInfo( CArchive& ar, bool loadingCache )
{
#if defined( SE_EDIT_SUPPORT )
	if( ar.IsStoring() )
	{
		if (CSynthEditDocBase::serializingMode != SERT_UNDO_SYSTEM)
		{
			for (auto it = module_list.begin(); it != module_list.end(); ++it)
			{
				if ((CSynthEditDocBase::serializingMode == SERT_SEM_CACHE && (*it).second->isDllAvailable()) || (*it).second->getSerialiseFlag())
				{
					//_RPTW1(_CRT_WARN, L"Caching %s\n", (*it).second->GetName().c_str() );
					ar << (*it).second;
				}
			}
		}

		ar << (Module_Info*) 0; // NULL marks last one
		//_RPTW0(_CRT_WARN, L"____________________________________\n" );
	}
	else
	{
		// load module infos stored in file, add any unknown ones to database
		Module_Info* mi;

		while(true)
		{
			ar >> mi;

			if( mi == 0 )
				break;

			// insert in module list, only if not already present
			auto res = module_list.insert( string_mod_info_pair( (mi->UniqueId()), mi) );

			if( res.second  )
			{
				// insert OK, module not previously registered
				mi->setLoadedIntoDatabase();

				// when loading cache, not finding existing info is fine.
				if( !loadingCache )
				{
					CSynthEditDocBase::CantLoad( mi->UniqueId() );
#if defined( _DEBUG )
					_RPTW1(_CRT_WARN, L"%s: Module info not avail, using stored info\n",  mi->UniqueId().c_str() );
#endif
				}
				else
				{
					//#if defined( _DEBUG )
					//	std::wstring t;
					//	t.Format( (L"[%s] Loaded"), mi->UniqueId() );
					//	_RPT1(_CRT_WARN, "%s\n", DebugCStringToAscii(t) );
					//#endif
				}
			}
			else
			{
				auto localModuleInfo = GetById(mi->UniqueId());

				// Check for wrapped VST2 plugins that are not available, only summary info.
				// In this case we prefer to use the full module information from the project file.
				if (!localModuleInfo->OnDemandLoad() && localModuleInfo->isSummary())
				{
					auto it = module_list.find(localModuleInfo->UniqueId());
					module_list.erase(it);
					delete localModuleInfo;

					auto res2 = module_list.insert(string_mod_info_pair((mi->UniqueId()), mi));

					assert(res2.second);
					{
						// insert OK, module not previously registered
						mi->setLoadedIntoDatabase();
					}
				}
				else
				{
					// store pointer so object can be deleted after load.
					{
						std::unique_ptr<Module_Info> ptr(mi);
						m_in_use_old_module_list.push_back(std::move(ptr));
					}

					if (!loadingCache)
					{
						// Check stored pin data compatible with local version of module (rough check). Ck_Waveshaper1 has this problem. two versions with same ID but different pin count.
						auto storedModuleInfo = mi;
						if (!isCompatibleWith(storedModuleInfo, localModuleInfo))
						{
							mi->m_incompatible_with_current_module = true;

							CSynthEditDocBase::upgradeLoadList.push_back(localModuleInfo->UniqueId());
						}
					}
				}
			}
		}
	}

#endif
}

void CModuleFactory::ExportModuleInfo(tinyxml2::XMLElement* documentE, ExportFormatType fileType)
{
#if defined( SE_EDIT_SUPPORT )
	if (CSynthEditDocBase::serializingMode != SERT_UNDO_SYSTEM)
	{
		auto doc = documentE->GetDocument();
		auto databaseX = doc->NewElement("PluginList");
		documentE->LinkEndChild(databaseX);

		for (auto& it : module_list)
		{
			if ((CSynthEditDocBase::serializingMode == SERT_SEM_CACHE && it.second->isDllAvailable()) || it.second->getSerialiseFlag())
			{
				it.second->Export(databaseX, fileType);
			}
		}
	}
#endif
}

#if defined( SE_EDIT_SUPPORT )

bool Module_Info::hasAttachedHostControls()
{
	for( module_info_pins_t::iterator it = gui_plugs.begin() ; it != gui_plugs.end() ; ++it )
	{
		if( HostControlAttachesToParentContainer( (*it).second->getHostConnect(0) ) )
		{
			return true;
		}
	}

	return false;
}
	
void Module_Info::FlagHostControls( bool* flaggedHostControls )
{
	for (auto it : gui_plugs)
	{
		auto hc = it.second->getHostConnect(0);
		if( hc != HC_NONE )
		{
			flaggedHostControls[hc] = true;
		}
	}
	for (auto it : plugs )
	{
		auto hc = it.second->getHostConnect(0);
		if (hc != HC_NONE)
		{
			flaggedHostControls[hc] = true;
		}
	}
	for (auto it : controller_plugs)
	{
		auto hc = it.second->getHostConnect(0);
		if (hc != HC_NONE)
		{
			flaggedHostControls[hc] = true;
		}
	}
}

#endif

void Module_Info::SetupPlugs()
{
	// old way, requires instansiating (new way done in contructor)
	// instansiate ug
	SetupPlugs_pt2( BuildSynthOb() );
	m_dsp_registered = true;
}

void Module_Info::SetupPlugs_pt2(ug_base* ug)
{
	if( ug != NULL )	// ignore display-only objects
	{
		InterfaceObjectArray temp;
		ug->ListInterface2( temp );	// and query it's interface
		delete ug;

		// set plug IDs
		for( int id = 0 ; id < (int) temp.size() ; id++ )
		{
			InterfaceObject* io = temp[id];
			io->setId(id);
			// care not to store pointer to variable belonging to deleted ug object.
			io->clearVariableAddress();

			if (io->isUiPlug(0))
			{
				gui_plugs.insert({ id, io });
			}

			if (!io->isUiPlug(0) || io->isDualUiPlug(0))
			{
				plugs.insert({ id, io });
			}

			// flag old-style gui connectors (require connecting in-Document)
			if( io->isUiPlug(0) )
			{
				io->SetFlags( io->GetFlags() | IO_OLD_STYLE_GUI_PLUG );
				// Fix for buggy "SL ExpScaler" "Param plug wrong direction"
			}

			/*
			#if defined( _DEBUG )
						if( !io->CheckEnumOnConnection(0) && io->GetDatatype() == DT_ENUM )
			{
				_RPTW1(_CRT_WARN, L"CheckEnumOnConnection FALSE %s.\n", this->GetName() );
			}
			#endif
			*/
			// parameters stored seperate.
			if( io->isParameterPlug(0) )
			{
				if( io->getParameterId(0) < 0 )
					io->setParameterId(0); // assuming my own modules use only one parameter max.

				if( io->getParameterFieldId(0) == FT_VALUE )
				{
					if( io->isUiPlug(0) && io->GetDirection() == DR_IN )
					{
						_RPT0(_CRT_WARN, "SDK2 module has PATCH-STORE on DR_IN pin!!.\n" );
						/*, no causes DUAL pins to not appear in DSP pins.
											int f = io->GetFlags();
											// note: clearing PS flag causes module to instansiate with non-sequential parameters.
											// i.e. 'KDL Volts2GuiFloat' has param 0,1, and 3 (but not 2).
											CLEAR_BITS( f, IO_UI_DUAL_FLAG|IO_PATCH_STORE);
											io->SetFlags( f );
						*/
					}
					else
					{
						RegisterSdk2Parameter( io );
					}
				}
			}
		}
	}
}

void Module_Info::RegisterSdk2Parameter( InterfaceObject* io )
{
	parameter_description* pd = GetOrCreateParameter( io->getParameterId( 0 ) );
	pd->name = io->GetName();
	pd->flags = IO_PARAMETER_PERSISTANT | (io->GetFlags() & (IO_PAR_PRIVATE|IO_PAR_POLYPHONIC|IO_PAR_POLYPHONIC_GATE|IO_HOST_CONTROL|IO_IGNORE_PATCH_CHANGE));
	pd->datatype = io->GetDatatype();
	//	pd->automation = io->automation();
	pd->defaultValue = io->GetDefault(0);

	if( pd->datatype == DT_ENUM )
	{
		pd->metaData = ( io->GetEnumList() );
	}
}

parameter_description* Module_Info::GetOrCreateParameter(int parameterId)
{
	module_info_parameter_t::iterator it = m_parameters.find( parameterId );

	if( it != m_parameters.end() )
	{
		return (*it).second;
	}

	parameter_description* param = new parameter_description();
	param->id = parameterId;

	bool res = m_parameters.insert( std::pair<int,parameter_description*>( param->id, param ) ).second;
	assert( res == true );
	return param;
}

InterfaceObject* Module_Info::getPinDescriptionByPosition(int p_id)
{
	int i = 0;

	for( module_info_pins_t::iterator it = plugs.begin() ; it != plugs.end() ; ++it )
	{
		if( i++ == p_id )
		{
			return (*it).second;
		}
	}

	return 0;
}

InterfaceObject* Module_Info::getPinDescriptionById(int p_id)
{
	module_info_pins_t::iterator it = plugs.find(p_id);

	if( it != plugs.end() )
	{
		return (*it).second;
	}

	// ok to return 0 with dynamic plugs. assert( m_dsp_registered && "DSP class not registered (included in se_vst project?" );
	return 0;
}

InterfaceObject* Module_Info::getGuiPinDescriptionByPosition(int p_index)
{
	int i = 0;

	for( auto it = gui_plugs.begin() ; it != gui_plugs.end() ; ++it )
	{
		if( i++ == p_index )
		{
			return (*it).second;
		}
	}

	return 0;
}

InterfaceObject* Module_Info::getGuiPinDescriptionById(int p_id)
{
	auto it = gui_plugs.find(p_id);

	if( it != gui_plugs.end() )
	{
		return (*it).second;
	}

	return 0;
}

parameter_description* Module_Info::getParameterById(int p_id)
{
	auto it = m_parameters.find(p_id);

	if( it != m_parameters.end() )
	{
		return (*it).second;
	}

	return 0;
}

parameter_description* Module_Info::getParameterByPosition(int index)
{
	int i = 0;

	for(auto it = m_parameters.begin() ; it != m_parameters.end() ; ++it )
	{
		if( i++ == index )
		{
			return (*it).second;
		}
	}

	return 0;
}

Module_Info* CModuleFactory::GetById(const std::wstring& p_id)
{
	auto it = module_list.find( p_id );

	if( it != module_list.end() )
	{
		return ( *it ).second;
	}
	else
	{
#ifdef _DEBUG
		if( Left(p_id,2).compare( L"BI" ) == 0 )
		{
			assert(false && "old-style no longer supported, use SE1.1 to upgrade project" );
		}
#endif
		//_RPTW1(_CRT_WARN, L"CModuleFactory::GetById(%s) can't Find. (included file?)\n", p_id.c_str() );
		return 0;
	}
}


#if defined(_DEBUG) && defined(SE_TARGET_PLUGIN)
bool CModuleFactory::debugInitCheck(const char* modulename)
{
	staticInitCheck2.push_back(modulename);
	return false;
}
#endif

// When running in a static library, this mechanism forces the linker to inlude all self-registering modules.
// without this the linker will decide that they can be discarded, since they are not explicity referenced elsewhere.
// This can also be run harmlessly in builds where it is not needed.
// see also SE_DECLARE_INIT_STATIC_FILE
#if defined(_DEBUG) && defined(SE_TARGET_PLUGIN)
// has extra debugging check
#define INIT_STATIC_FILE(filename) void se_static_library_init_##filename(); se_static_library_init_##filename(); staticInitCheck.push_back( #filename );
#else
#define INIT_STATIC_FILE(filename) void se_static_library_init_##filename(); se_static_library_init_##filename();
#endif

void CModuleFactory::initialise_synthedit_modules(bool passFalse)
{
#ifndef _DEBUG
	// NOTE: We don't have to actually call these functions (they do nothing), only *reference* them to
	// cause them to be linked into plugin such that static objects get initialised.
    if(!passFalse)
        return;
#endif

#if GMPI_IS_PLATFORM_JUCE==1
	INIT_STATIC_FILE(ADSR);
	INIT_STATIC_FILE(BpmClock3);
	INIT_STATIC_FILE(BPMTempo);
	INIT_STATIC_FILE(ButterworthHP);
	INIT_STATIC_FILE(Converters);
	INIT_STATIC_FILE(FreqAnalyser2);
	INIT_STATIC_FILE(IdeLogger);
    INIT_STATIC_FILE(ImpulseResponse);
    INIT_STATIC_FILE(Inverter);
	INIT_STATIC_FILE(MIDItoGate);
	INIT_STATIC_FILE(NoteExpression);
	INIT_STATIC_FILE(OscillatorNaive);
	INIT_STATIC_FILE(PatchMemoryBool);
	INIT_STATIC_FILE(PatchMemoryBool);
	INIT_STATIC_FILE(PatchMemoryFloat);
	INIT_STATIC_FILE(PatchMemoryInt);
	INIT_STATIC_FILE(PatchMemoryList3);
	INIT_STATIC_FILE(PatchMemoryText);
	INIT_STATIC_FILE(Slider);
	INIT_STATIC_FILE(SoftDistortion);
	INIT_STATIC_FILE(Switch);
	INIT_STATIC_FILE(UnitConverter);
	INIT_STATIC_FILE(VoltsToMIDICC);
	INIT_STATIC_FILE(Waveshaper3Xp);
	INIT_STATIC_FILE(Waveshapers);
	INIT_STATIC_FILE(UserSettingText); // not generated automatically at present
	INIT_STATIC_FILE(UserSettingText_Gui);
	INIT_STATIC_FILE(UserSettingText_Controller);

//	INIT_STATIC_FILE(Dynamics);
//	INIT_STATIC_FILE(VoltMeter);
//	INIT_STATIC_FILE(SignalLogger);
#else
	INIT_STATIC_FILE(MidiToCv2);
	INIT_STATIC_FILE(ug_denormal_detect);
	INIT_STATIC_FILE(ug_denormal_stop);
	INIT_STATIC_FILE(ug_filter_allpass);
	INIT_STATIC_FILE(ug_filter_biquad);
	INIT_STATIC_FILE(ug_filter_sv);
	INIT_STATIC_FILE(ug_logic_decade);
	INIT_STATIC_FILE(ug_logic_not);
	INIT_STATIC_FILE(ug_logic_shift);
	INIT_STATIC_FILE(ug_math_ceil);
	INIT_STATIC_FILE(ug_math_floor);
	INIT_STATIC_FILE(ug_test_tone);
	INIT_STATIC_FILE(ug_logic_Bin_Count);

#endif

#if defined( SE_TARGET_PLUGIN )
	INIT_STATIC_FILE(ug_vst_in);
	INIT_STATIC_FILE(ug_vst_out);
#endif

	INIT_STATIC_FILE(PatchPoints);
	//	INIT_STATIC_FILE(Scope3);
	INIT_STATIC_FILE(VoiceMute);
	INIT_STATIC_FILE(ug_adsr);
	INIT_STATIC_FILE(ug_adder2);
	INIT_STATIC_FILE(ug_clipper);
	INIT_STATIC_FILE(ug_combobox);
	INIT_STATIC_FILE(ug_comparator);
	INIT_STATIC_FILE(ug_container);
	INIT_STATIC_FILE(ug_cross_fade);
	INIT_STATIC_FILE(ug_cv_midi);
	INIT_STATIC_FILE(ug_default_setter);
	INIT_STATIC_FILE(ug_delay);
	INIT_STATIC_FILE(ug_feedback_delays);
	INIT_STATIC_FILE(ug_filter_1pole);
	INIT_STATIC_FILE(ug_filter_1pole_hp);
	INIT_STATIC_FILE(ug_fixed_values);
	INIT_STATIC_FILE(ug_float_to_volts);
	INIT_STATIC_FILE(ug_generic_1_1);
	INIT_STATIC_FILE(ug_io_mod);
	//    INIT_STATIC_FILE(ug_led);
	INIT_STATIC_FILE(ug_logic_counter);
    INIT_STATIC_FILE(StepSequencer3); // also in ug_logic_counter.cpp
   
	INIT_STATIC_FILE(ug_logic_gate);
	INIT_STATIC_FILE(ug_math_base);
	INIT_STATIC_FILE(ug_midi_automator);
	INIT_STATIC_FILE(ug_midi_controllers);
	INIT_STATIC_FILE(ug_midi_filter);
	//     INIT_STATIC_FILE(ug_midi_monitor);
	INIT_STATIC_FILE(ug_midi_to_cv);
	INIT_STATIC_FILE(ug_monostable);
	INIT_STATIC_FILE(ug_multiplier);
	INIT_STATIC_FILE(ug_oscillator2);
	INIT_STATIC_FILE(ug_oscillator_pd);
	INIT_STATIC_FILE(ug_pan);
	INIT_STATIC_FILE(ug_patch_param_setter);
	INIT_STATIC_FILE(ug_patch_param_watcher);
	INIT_STATIC_FILE(ug_peak_det);
	INIT_STATIC_FILE(ug_quantiser);
	INIT_STATIC_FILE(ug_random);
	INIT_STATIC_FILE(ug_sample_hold);
	INIT_STATIC_FILE(ug_slider);
	INIT_STATIC_FILE(ug_switch);
	INIT_STATIC_FILE(ug_system_modules);
	INIT_STATIC_FILE(ug_text_entry);
	INIT_STATIC_FILE(ug_vca);
	INIT_STATIC_FILE(ug_voice_monitor);
	INIT_STATIC_FILE(ug_voice_splitter);
//	INIT_STATIC_FILE(ug_volt_meter);
	INIT_STATIC_FILE(ug_voltage_to_enum);
	INIT_STATIC_FILE(ug_volts_to_float);

	INIT_STATIC_FILE(DAWSampleRate);
	INIT_STATIC_FILE(StepSequencer2);
	INIT_STATIC_FILE(MIDI2Converter)
	INIT_STATIC_FILE(MPEToMIDI2)

	// when the UI is defined in JUCE, not SynthEdit, we don't include GUI modules
#ifndef SE_USE_JUCE_UI
	INIT_STATIC_FILE(Slider_Gui)
	INIT_STATIC_FILE(ListEntry_Gui);
	INIT_STATIC_FILE(PatchPointsGui)
	INIT_STATIC_FILE(PlainImage_Gui)
	INIT_STATIC_FILE(RegistrationCheck) // has DSP also,but too bad.
#endif

	// You can include extra plugin-specific modules by placing this define in projucer 'Extra Preprocessor Definitions'
	// e.g. SE_EXTRA_STATIC_FILE_CPP="../PROJECT_NAME/Resources/module_static_link.cpp"
#ifdef SE_EXTRA_STATIC_FILE_CPP
#include SE_EXTRA_STATIC_FILE_CPP
#endif

	initialise_synthedit_extra_modules(passFalse);
}
