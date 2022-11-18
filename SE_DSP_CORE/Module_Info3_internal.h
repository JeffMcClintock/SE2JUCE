#pragma once

#include "Module_Info3_base.h"
#include "./modules/se_sdk3/mp_sdk_common.h"

typedef std::map< int, MP_CreateFunc2> FactoryMethodList2_t;


class Module_Info3_internal : public Module_Info3_base
{
public:
	Module_Info3_internal(const wchar_t* moduleId);
	Module_Info3_internal(const char* xml);

	int32_t RegisterPluginConstructor( int subType, MP_CreateFunc2 create );
	virtual bool fromExternalDll(){ return false;}

	ug_base* BuildSynthOb();
	virtual gmpi::IMpUnknown* Build(int subType, bool quietFail = false);

#if defined( SE_EDIT_SUPPORT )
#if 0
	class TiXmlElement* ExportXml(TiXmlElement* element, enum ExportFormatType format, const std::string& overrideModuleId, const std::string& overrideModuleName)
	{
		// Because these modules are built-in to the executable, they register their XML in VST3 (unlike SEMs), so don't need to serialise XML.
		if (format != SAT_VST3)
		{
			return Module_Info3_base::ExportXml(element, format, overrideModuleId, overrideModuleName);
		}
		return nullptr;
	}
#endif
	virtual tinyxml2::XMLElement* Export(tinyxml2::XMLElement* element, ExportFormatType format, const std::string & overrideModuleId, const std::string & overrideModuleName) override
	{
		// Because these modules are built-in to the executable, they register their XML in VST3 (unlike SEMs), so don't need to serialise XML.
		if (format != SAT_VST3)
		{
			return Module_Info3_base::Export(element, format, overrideModuleId, overrideModuleName);
		}
		return nullptr;
	}
#endif

protected:
	Module_Info3_internal() {} // Serialising.

#if defined( SE_SUPPORT_MFC )
	DECLARE_SERIAL( Module_Info3_internal )
	virtual void Serialize( CArchive& ar );
#endif

	FactoryMethodList2_t factoryMethodList_; // new way.
};



