#pragma once

#include <map>
#include "conversion.h"
#include "plug_description.h"
#include "mfc_emulation.h"
#include "ug_flags.h"
#include "SerializationHelper_XML.h"

namespace tinyxml2
{
	class XMLElement;
}

typedef std::map<int, class InterfaceObject*> module_info_pins_t;
typedef std::map<int, struct parameter_description*> module_info_parameter_t;

enum {MT_INTERNAL, MT_SDK2, MT_SDK3 }; // module technology types
enum MpWindowTypes { MP_WINDOW_TYPE_NONE, MP_WINDOW_TYPE_HWND, MP_WINDOW_TYPE_COMPOSITED, MP_WINDOW_TYPE_WPF, MP_WINDOW_TYPE_WPF_INTERNAL, MP_WINDOW_TYPE_VSTGUI, MP_WINDOW_TYPE_XP, MP_WINDOW_TYPE_CADMIUM
};

// This class ties together the CUG interface object and the ug_base based synthesis object

class Module_Info : public CObject
{
protected:
	void RegisterParameters(class TiXmlNode* parameters);
	void RegisterPins(TiXmlNode* plugin_data, int32_t plugin_sub_type);
	void RegisterParameters(tinyxml2::XMLElement* parameters);
	void RegisterPins(tinyxml2::XMLElement* plugin_data, int32_t plugin_sub_type);
	void ScanPinXml(tinyxml2::XMLElement* xmlObjects, module_info_pins_t * pinlist, int32_t plugin_sub_type);
	void ScanPinXml(TiXmlNode* xmlObjects, module_info_pins_t* pinlist, int32_t plugin_sub_type);
	void RegisterPin(class TiXmlElement* pin, module_info_pins_t* pinlist, int32_t plugin_sub_type, int& nextPinId);

	void RegisterPin(tinyxml2::XMLElement * pin, module_info_pins_t * pinlist, int32_t plugin_sub_type, int & pin_id);
	virtual int getClassType() {return 1;} // 0 - Module_Info3, 1 - Module_Info, 2 - Module_Info3_internal, 3 - Module_Info_Plugin
	
public:
	bool scanned_xml_dsp;
	bool scanned_xml_gui;
	bool scanned_xml_parameters;

#if defined( _DEBUG)
	bool load_failed_gui = false;
#endif

	Module_Info(const std::wstring& p_unique_id); // new, SDK3-based
	Module_Info(ug_base *(*ug_create)(), const char* xml); // new, Internal

	Module_Info(const wchar_t* p_unique_id, int p_sid_name, int p_sid_group_name, class CDocOb *( *cug_create )( Module_Info* ), class ug_base *( *ug_create )( ), int p_flags);
	Module_Info(const wchar_t* p_unique_id, const wchar_t* name, const wchar_t* group_name, class CDocOb *( *cug_create )( Module_Info* ), class ug_base *( *ug_create )( ), int p_flags);

	virtual ~Module_Info();

	virtual void ScanXml(class TiXmlElement* xmlData);

	const std::wstring& UniqueId() const
	{
		return m_unique_id;
	}
	virtual std::wstring Filename()
	{
		return L"SynthEdit.exe";
	}

	bool gui_object_non_visible();

	// New way.
	virtual class gmpi::IMpUnknown* Build(int /* family */, bool /*quietFail*/ = false){ return 0; }
	// Old way.
	virtual class ug_base* BuildSynthOb();
	//virtual class CDocOb* BuildDocObject()
	//{
	//	return doc_create(this);
	//}

	virtual std::wstring GetGroupName();

	virtual std::wstring GetName();

	virtual std::wstring GetDescription()
	{
		return (L"");
	}

	std::wstring GetHelpUrl();
#if defined( SE_EDIT_SUPPORT )
	virtual std::wstring showSdkVersion();
	virtual bool isSummary() // for VST2 plugins held as name-only.
	{
		return false;
	}
#endif

	class InterfaceObject* getPinDescriptionByPosition(int index);

	class InterfaceObject* getGuiPinDescriptionByPosition(int index);

	class InterfaceObject* getPinDescriptionById(int id); // NOT index, unique ID.

	class InterfaceObject* getGuiPinDescriptionById(int id);

	parameter_description* getParameterById(int p_id);

	parameter_description* getParameterByPosition(int index);

	void RegisterSdk2Parameter( InterfaceObject* io );

	parameter_description* GetOrCreateParameter(int parameterId);

	int GuiPlugCount()
	{
		return (int)gui_plugs.size();
	}

	int GetFlags()
	{
		return flags;
	}

	int PlugCount()
	{
		return (int)plugs.size();
	}

	void SetupPlugs();

	void SetupPlugs_pt2(ug_base* ug);

	void SetSerialiseFlag()
	{
		m_serialise_me= true;
	}

	void ClearSerialiseFlag()
	{
		m_serialise_me= false;
	}

	bool getSerialiseFlag();

	void setLoadedIntoDatabase(bool loaded = true)
	{
		m_loaded_into_database=loaded;
	}

	bool getLoadedIntoDatabase()
	{
		return m_loaded_into_database;
	}

	bool m_gui_registered = false;
	bool m_dsp_registered = false;

	virtual int ModuleTechnology()
	{
		return MT_INTERNAL;
	}
	virtual int getWindowType() // SDK3 window type only.
	{
		return MP_WINDOW_TYPE_NONE; // no *SDK3* window.
	}

	bool isDllAvailable();

	virtual bool OnDemandLoad();

	bool hasDspModule();

	virtual bool FileInUse()
	{
		return false;
	}

	virtual bool fromExternalDll(){ return false;}

#if defined( SE_EDIT_SUPPORT )
	bool hasGuiModule();
	void SaveModuleInfoPinXml(InterfaceObject* pin, ExportFormatType format, TiXmlElement* DspXml, int& expectedId);
	void SaveModuleInfoPinXml(InterfaceObject* pin, ExportFormatType format, tinyxml2::XMLElement* DspXml, int& expectedId);
	virtual tinyxml2::XMLElement* Export(tinyxml2::XMLElement* element, ExportFormatType format, const std::string& overrideModuleId = "", const std::string& overrideModuleName = "");
	virtual void Import(tinyxml2::XMLElement* element, ExportFormatType format);
	bool hasAttachedHostControls();
	void FlagHostControls( bool* flaggedHostControls );
	// Module info loaded from project file is different than local module. Plugs differ.
	// Indicates need for 'smart' upgrading.
	bool m_incompatible_with_current_module = false;
#endif

protected:

	Module_Info(); // serialisation only

	DECLARE_SERIAL( Module_Info )

#if defined( SE_EDIT_SUPPORT )
	virtual void Serialize( CArchive& ar );
#endif

//	CDocOb* (*doc_create)( Module_Info* p_type );

	ug_base* (*dsp_create)();

	void ClearPlugs();

	std::wstring m_unique_id;
	int sid_name;	// resource string id for localised name
	int sid_group;
	int flags;		// flags to denote special properties (accessable only internally) CF_* see also ug_flags

	// alternate method of storing name, not using resource-based strings (eases porting to SDK)
	std::wstring m_name;
	std::wstring m_group_name;
	std::wstring helpUrl_;

public:

	// new
	void Register(struct module_description& mod, get_pin_properties_func pin_func); // GUI
	void Register(struct module_description_internal& p_module_desc); // DSP (old)
	void Register(struct module_description_dsp& p_module_desc); // DSP (pure)

	module_info_pins_t controller_plugs;
	module_info_pins_t gui_plugs;
	module_info_pins_t plugs;
	module_info_parameter_t m_parameters;
	std::vector<patchpoint_description> patchPoints;

	int latency;

	void SetUnavailable()
	{
		m_module_dll_available=false;
	}

	void setShellPlugin();
	bool isShellPlugin();

protected:
	bool m_module_dll_available = {}; // Is the SEM present on local system? (or just a placeholder).

private:
	bool m_serialise_me = false; // flags if this info needs storing in project file (because this module used in patch)
	bool m_loaded_into_database;
};
