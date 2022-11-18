#pragma once

#include "Module_Info3_base.h"
#include "mfc_emulation.h"

#ifndef _WIN32
#include <dlfcn.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "./modules/shared/xp_dynamic_linking.h"
#include "SerializationHelper_XML.h"

class Module_Info3 : public Module_Info3_base
{
public:
	Module_Info3( const std::wstring& file_and_dir, const std::wstring& overridingCategory = L"" );
	~Module_Info3();

	virtual bool OnDemandLoad() override
	{
		return !LoadDllOnDemand();
	}
	virtual bool isSummary() // for VST2 plugins held as name-only.
	{
		return m_parameters.empty() && gui_plugs.empty() && plugs.empty(); // reasonable guess.
	}

	void LoadDll();
	void ReLoadDll();
	void Unload();
	virtual std::wstring Filename() override
	{
		return filename;
	}

	gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> getFactory2();

	virtual ug_base* BuildSynthOb() override;
//	virtual class CDocOb* BuildDocObject() override;
	virtual gmpi::IMpUnknown* Build(int subType, bool quietFail) override;

	virtual bool FileInUse() override
	{
    #if defined(_WIN32)
		return dllHandle != 0;
        #else
        assert(false);
        return false;
        #endif
	}
	virtual bool fromExternalDll() override { return true;};

#if defined( SE_EDIT_SUPPORT )
	virtual std::wstring showSdkVersion();
#endif

protected:
	Module_Info3(); // serialisation only

#if defined( SE_EDIT_SUPPORT )
	DECLARE_SERIAL( Module_Info3 )
	virtual void Serialize( CArchive& ar );
#endif

	std::pair< gmpi_dynamic_linking::DLL_HANDLE, std::wstring > getDllInfo()
	{
		return std::make_pair(dllHandle, filename);
	}
	
#if defined( SE_EDIT_SUPPORT )
	tinyxml2::XMLElement* Export(tinyxml2::XMLElement* element, ExportFormatType format, const std::string& overrideModuleId = "", const std::string& overrideModuleName = "") override
	{
		auto pluginXml = Module_Info3_base::Export(element, format, overrideModuleId, overrideModuleName);

		if (format == SAT_SEM_CACHE)
		{
			pluginXml->SetAttribute("file", WStringToUtf8(filename).c_str() );
		}

		return pluginXml;
	}
#endif

	bool LoadDllOnDemand();

private:

	gmpi_dynamic_linking::DLL_HANDLE dllHandle;
	std::wstring filename;
};
