#pragma once

#include "module_info.h"
#include <string>

class Module_Info3_base : public Module_Info
{
public:
	Module_Info3_base( const wchar_t* moduleId );
	void ScanXml( class TiXmlElement* xmlData ) override;
#if defined( SE_EDIT_SUPPORT )
	tinyxml2::XMLElement* Export(tinyxml2::XMLElement* element, ExportFormatType format, const std::string& overrideModuleId = "", const std::string& overrideModuleName = "") override;
	void Import(tinyxml2::XMLElement* pluginE, ExportFormatType format) override;
	void CompareXml(class TiXmlNode* original, TiXmlNode* copy, std::string indent = "   ");

	// Track location of Mac SEM, for mac export only.
	std::wstring macSemBundlePath;
	void setMacSemPath(std::wstring pmacSemBundlePath)
	{
		macSemBundlePath = pmacSemBundlePath;
	}
#endif
	virtual ug_base* BuildSynthOb( void ) override;

	void SetupPlugs();
	virtual std::wstring GetDescription() override
	{
		return (m_description);
	}
	int ModuleTechnology() override;
	virtual std::wstring showSdkVersion();
	virtual int getWindowType() override
	{
		return m_window_type;
	}

protected:
#if defined( SE_EDIT_SUPPORT )
	virtual void Serialize(CArchive& ar);
#endif

	Module_Info3_base(); //serialisation.

	int ug_flags;
	std::wstring m_description;
	int m_window_type = MP_WINDOW_TYPE_NONE;
};
