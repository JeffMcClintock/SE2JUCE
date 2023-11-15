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
	bool fromExternalDll() override { return false;}

	ug_base* BuildSynthOb() override;
	gmpi::IMpUnknown* Build(int subType, bool quietFail = false) override;

#if defined( SE_EDIT_SUPPORT )
	tinyxml2::XMLElement* Export(tinyxml2::XMLElement* element, ExportFormatType format, const std::string & overrideModuleId, const std::string & overrideModuleName) override
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
	int getClassType() override { return 2; } // 0 - Module_Info3, 1 - Module_Info, 2 - Module_Info3_internal, 3 - Module_Info_Plugin

#if defined( SE_SUPPORT_MFC )
	DECLARE_SERIAL( Module_Info3_internal )
	virtual void Serialize( CArchive& ar );
#endif

	FactoryMethodList2_t factoryMethodList_; // new way.
};



