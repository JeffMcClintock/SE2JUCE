#pragma once
#include <map>
#include <string>
#include <vector>
#include <list>
#include <mutex>
#include "module_register.h"
#include "module_info.h"
#include "modules/se_sdk3/mp_sdk_common.h"
#include "SerializationHelper_XML.h"

#define ModuleFactory() CModuleFactory::Instance()

// this class is a singleton,
// it's shared between all instances of the dll (in VST mode)
class CModuleFactory
{
	friend class CSynthEditApp;

public:
	static CModuleFactory* Instance();
	gmpi::IMpUnknown* Create( int32_t subType, const std::wstring& uniqueId );
	Module_Info* GetById( const std::wstring& p_id );
	int32_t RegisterPlugin(int subType, const wchar_t* uniqueId, MP_CreateFunc2 create); // newest.
	int32_t RegisterPluginWithXml(int subType, const char* xml, MP_CreateFunc2 create); // newest.
	bool RegisterModule( get_module_properties_func mod_func, get_pin_properties_func pin_func );
	bool RegisterModule( module_description_internal& p_module_desc );
	bool RegisterModule( module_description_dsp& p_module_desc );
	bool RegisterModule( Module_Info* p_module_info );
	void RegisterPluginsXml( const char* xmlFile );
	void RegisterPluginsXml( class TiXmlNode* pluginList );
#if defined( SE_EDIT_SUPPORT )
	Module_Info* GetByIdSerializing(const std::wstring& p_id);
	void RegisterExternalPluginsXml(class TiXmlDocument* doc, const std::wstring& full_path, const std::wstring& group_name, bool isShellPlugin = false);
#else
	void RegisterExternalPluginsXmlOnce(TiXmlNode*);
#endif

	// file path management
	std::vector< std::wstring > PrefabFileNames; // list of prefabs and VST plugins

	void ClearSerialiseFlags(bool isExportingPlugin = false);
	void SerialiseModuleInfo(CArchive& ar, bool loadingCache = false);
	void ImportModuleInfo(tinyxml2::XMLElement* documentE, ExportFormatType targetType, int fileFormatVersion);
	void ExportModuleInfo(tinyxml2::XMLElement* documentE, ExportFormatType targetType);

	// Editor support.
	void ClearModuleDb( const std::wstring& p_extension ); // clears either prefabs or vst plugins depending on extension arg.

	std::map<std::wstring, Module_Info*> module_list; // maps menu id to module identifier
#if defined(_DEBUG) && defined(SE_TARGET_PLUGIN)
	bool debugInitCheck(const char* modulename);
	std::vector<std::string> staticInitCheck; // list of modules with SE_DECLARE_INIT_STATIC_FILE
	std::vector<std::string> staticInitCheck2; // list of modules with INIT_STATIC_FILE
#endif
#if defined( _DEBUG)
	std::vector<std::wstring> failedGuiModules;
	std::string GetFailedGuiModules();
#endif

private:
#ifndef SE_EDIT_SUPPORT
	bool initializedFromXml = {};
	std::mutex RegisterExternalPluginsXmlOnce_lock;
#endif
	CModuleFactory();
	CModuleFactory( const CModuleFactory& );
	~CModuleFactory();
	Module_Info* FindOrCreateModuleInfo( const std::wstring& p_unique_id );
	void initialise_synthedit_modules(bool passFalse = false);
};
