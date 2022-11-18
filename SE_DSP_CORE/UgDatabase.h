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

// Flags for special CUG types CUG::getType()->GetFlags()
//#define CF_CONTAINER    1
//#define CF_HAS_IO_PLUGS 2
#define CF_IO_MOD       4

// don't show ug on insert menu
#define CF_HIDDEN       8

// This UG controls polyphony (!!could use unit_gen flag UGF_POLYPHONIC_GENERATOR?)
#define CF_NOTESOURCE   32

// ANY CONTAINER WITH A NOTESOURCE/Automator etc can't be expanded 'inline'
// because each container assumes only one.
#define CF_DONT_EXPAND_CONTAINER    64

// visible on control panel
#define CF_PANEL_VIEW               0x0080

// visible on structure view
#define CF_STRUCTURE_VIEW           0x0100
#define CF_DUMMY_VIEW				0x0200
#define CF_AUTOMATION_VIEW          0x0800
#define CF_OLD_STYLE_LISTINTERFACE  0x1000
//#define CF_NON_VISIBLE              0x2000

// Patch select decorator is special...
//#define CF_DRAW_AT_PANEL_TOP        0x4000
#define CF_DEBUG_VIEW               0x8000

#define CF_SHELL_PLUGIN             0x10000
#define CF_IS_FEEDBACK              0x20000

// ONLY PIGYBACKED DURING SERIALISE TO MAINTAIN FILE-FORMAT.
#define CF_RACK_MODULE_FLAG         0x40000

#define CF_ALWAYS_EXPORT			(1 << 25)	// e.g. Voice-Mute.


// structure modules, using default CUG as GUI class
#define REGISTER_MODULE_1( module_id, sid_name, sid_group, ug, flags, desc ) bool res_##ug = ModuleFactory()->RegisterModule(new Module_Info(module_id, sid_name, sid_group, 0, ug::CreateObject, flags) );

// same (backward compatible)
#define REGISTER_MODULE_1_BC( old_type_id, module_id, sid_name, sid_group, ug, flags, desc ) bool res_##old_type_id = ModuleFactory()->RegisterModule(new Module_Info(module_id, sid_name, sid_group, 0, ug::CreateObject, flags) );

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
	void RegisterExternalPluginsXml(class TiXmlDocument* doc, const std::wstring& full_path, const std::wstring& group_name, bool isShellPlugin = false);
#else
	void RegisterExternalPluginsXmlOnce(TiXmlNode*);
#endif

	// file path management
	std::vector< std::wstring > PrefabFileNames; // list of prefabs and VST plugins

	void ClearSerialiseFlags( void );
	void SerialiseModuleInfo(CArchive& ar, bool loadingCache = false);
	void ImportModuleInfo(tinyxml2::XMLElement* documentE, ExportFormatType targetType);
	void ExportModuleInfo(tinyxml2::XMLElement* documentE, ExportFormatType targetType);

	// Editor support.
	void ClearModuleDb( const std::wstring& p_extension ); // clears either prefabs or vst plugins depending on extension arg.

	std::map<std::wstring, Module_Info*> module_list; // maps menu id to module identifier
#if defined(_DEBUG) && defined(SE_TARGET_PLUGIN)
	bool debugInitCheck(const char* modulename);
	std::vector<std::string> staticInitCheck; // list of modules with SE_DECLARE_INIT_STATIC_FILE
	std::vector<std::string> staticInitCheck2; // list of modules with INIT_STATIC_FILE
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
