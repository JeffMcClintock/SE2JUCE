/* 
#include "SerializationHelper_XML.h"

// useage

	// Add a template member in header that serializes all relevant members.
	// Serializer will be one of {XmlSaveHelper, XmlLoadHelper}

	template< class Serializer >
	void SerialiseB(Archive2& ar)
	{
		// BaseClass::SerialiseB(ar); // if relevant

		Serializer s(ar.xmlElement);

		s("shortName", m_short_name);
	}

	// add a override of Serialize2 that instansiates the appropriate template
	void Serialize2(Archive2& ar) override
	{
		if (ar.isSaving)
			SerialiseB<XmlSaveHelper>(ar);
		else
			SerialiseB<XmlLoadHelper>(ar);
	}

	// call Serialize2 when you need to export/import the object

void PatchParameter_base::Export(tinyxml2::XMLElement* parameters_xml, ExportFormatType targetType)
{
	{
		std::unique_ptr<tinyxml2::XMLElement> parameter_xml(parameters_xml->GetDocument()->NewElement("param"));

		XmlSaveHelper helper(parameters_xml);
		Archive2 ar{ true , parameter_xml.get(), targetType };
		Serialise2(ar);

		parameters_xml->LinkEndChild(parameter_xml.release());
	}


*/

#pragma once
#include "modules/tinyXml2/tinyxml2.h"
#include "conversion.h"

enum ExportFormatType {
	 SAT_SYNTHEDIT_DSP
	,SAT_VST3
	,SAT_SYNTHEDIT_GUI_STRUCT
	,SAT_SYNTHEDIT_GUI_PANEL
	,SAT_SUBCONTROLS_GUI
	,SAT_VST3_PARAMETERS
	,SAT_VST3_CONTROLERS
	,SAT_CODE_SKELETON
	,SAT_SYNTHEDIT_DOCUMENT
	,SAT_SEM_CACHE
	,SAT_CADMIUM_VIEW
};


/*
* todo: Replace ExportFormatType with flags, which should be more flexible.
SAT_SYNTHEDIT_DSP => EXP_DSP | EXP_EDITOR
SAT_SYNTHEDIT_GUI_STRUCT => EXP_GUI |EXP_STRUCT |EXP_EDITOR
SAT_SYNTHEDIT_GUI_PANEL => EXP_GUI |EXP_PANEL |EXP_EDITOR
SAT_VST3_CONTROLERS => EXP_CONTROLERS |EXP_VST3
*/
enum ExportFormatTypeFlags {
	 EXP_DSP			= 1 << 0
	,EXP_DOCUMENT		= 1 << 1
	,EXP_PARAMETERS		= 1 << 2
	,EXP_CONTROLERS		= 1 << 3

	,EXP_CODE_SKELETON	= 1 << 4
	,EXP_SEM_CACHE		= 1 << 5

	,EXP_PANEL			= 1 << 10
	,EXP_STRUCT			= 1 << 11
	,EXP_CADMIUM_VIEW	= 1 << 12

	,EXP_EDITOR			= 1 << 13
	,EXP_PLUGIN			= 1 << 14

	,EXP_VST3			= 1 << 15
	,EXP_AU				= 1 << 16
	,EXP_JUCE			= 1 << 17
};

struct Archive2
{
	bool isSaving;// = true;
	tinyxml2::XMLElement* xmlElement;// = nullptr;
	ExportFormatType targetType;// = SAT_SYNTHEDIT_DOCUMENT;
};

#define DECLARE_SERIAL2 	void Serialize2(Archive2& ar) override \
{ \
	CUG::Serialize2(ar); \
	\
	if (ar.isSaving) \
		SerialiseB<XmlSaveHelper>(ar); \
	else \
		SerialiseB<XmlLoadHelper>(ar); \
}

struct XmlSaveHelper
{
	/* bit messy
	// value to xml
	template< typename T >
	auto& toCompatible(const T& value)
	{
		return value;
	}

	template<>
	auto& toCompatible(const std::wstring& value)
	{
		static std::string temp;
		temp = WStringToUtf8(value);
		return temp.c_str();
	}
	*/

	tinyxml2::XMLElement* XmlParent;
	int errorCode = 0;
	XmlSaveHelper(tinyxml2::XMLElement* pXmlParent) : XmlParent(pXmlParent) {}
	XmlSaveHelper(Archive2& ar) : XmlParent(ar.xmlElement) {}

	template< typename T >
	void operator()(const char* name, const T& value)
	{
		if(value != T{})
		{
			XmlParent->SetAttribute(name, value);
		}
	}

	template< typename T >
	void operator()(const char* name, const T& value, T defaultValue)
	{
		if(value != defaultValue)
		{
			XmlParent->SetAttribute(name, value);
		}
	}

	template<>
	void operator()(const char* name, const std::wstring& value)
	{
		if (!value.empty())
		{
			XmlParent->SetAttribute(name, WStringToUtf8(value).c_str());
		}
	}

	template<>
	void operator()(const char* name, const std::wstring& value, std::wstring defaultValue)
	{
		if (value != defaultValue)
		{
			XmlParent->SetAttribute(name, WStringToUtf8(value).c_str());
		}
	}

	template<>
	void operator()(const char* name, const std::string& value)
	{
		if (!value.empty())
		{
			XmlParent->SetAttribute(name, value.c_str());
		}
	}

	template<>
	void operator()(const char* name, const std::string& value, std::string defaultValue)
	{
		if (value != defaultValue)
		{
			XmlParent->SetAttribute(name, value.c_str());
		}
	}

#ifdef _MFC_VER
	// MFC types Support.
	template<>
	void operator()(const char* name, const CRect& value)
	{
		auto xml = XmlParent->GetDocument()->NewElement(name);
		XmlParent->LinkEndChild(xml);
		xml->SetAttribute("l", (int) value.left);
		xml->SetAttribute("r", (int) value.right);
		xml->SetAttribute("t", (int) value.top);
		xml->SetAttribute("b", (int) value.bottom);
	}

	template<>
	void operator()(const char* name, const CPoint& value)
	{
		auto xml = XmlParent->GetDocument()->NewElement(name);
		XmlParent->LinkEndChild(xml);
		xml->SetAttribute("x", (int) value.x);
		xml->SetAttribute("y", (int) value.y);
	}

	template<>
	void operator()(const char* name, const CSize& value)
	{
		auto xml = XmlParent->GetDocument()->NewElement(name);
		XmlParent->LinkEndChild(xml);
		xml->SetAttribute("cx", (int) value.cx);
		xml->SetAttribute("cy", (int) value.cy);
	}

#endif // MFC
};

struct XmlLoadHelper
{
	tinyxml2::XMLElement* XmlParent;
	tinyxml2::XMLError errorCode = tinyxml2::XML_SUCCESS;
	XmlLoadHelper(tinyxml2::XMLElement* pXmlParent) : XmlParent(pXmlParent) {}
	XmlLoadHelper(Archive2& ar) : XmlParent(ar.xmlElement) {}

	template< typename T >
	void operator()(const char* name, T& value)
	{
		if(tinyxml2::XML_SUCCESS != XmlParent->QueryAttribute(name, &value))
		{
			value = T{};
		}
	}

	template< typename T >
	void operator()(const char* name, T& value, T defaultValue)
	{
		if(tinyxml2::XML_SUCCESS != XmlParent->QueryAttribute(name, &value))
		{
			value = defaultValue;
		}
	}

	template<>
	void operator()(const char* name, std::wstring& value)
	{
		const char* temp{};
		errorCode = XmlParent->QueryStringAttribute(name, &temp);
		if (errorCode == tinyxml2::XML_SUCCESS)
		{
			value = Utf8ToWstring(temp);
		}
		else
		{
			value = {};
		}
	}

	template<>
	void operator()(const char* name, std::wstring& value, std::wstring defaultValue)
	{
		const char* temp{};
		errorCode = XmlParent->QueryStringAttribute(name, &temp);
		if (errorCode == tinyxml2::XML_SUCCESS)
		{
			value = Utf8ToWstring(temp);
		}
		else
		{
			value = defaultValue;
		}
	}

	template<>
	void operator()(const char* name, std::string& value)
	{
		const char* temp{};
		errorCode = XmlParent->QueryStringAttribute(name, &temp);
		if (errorCode == tinyxml2::XML_SUCCESS)
		{
			value = temp;
		}
		else
		{
			value = {};
		}
	}

	/*
	template<>
	void operator()(const char* name, bool& value)
	{
		int temp{};
		errorCode = (tinyxml2::XMLError) XmlParent->QueryAttribute(name, &temp);
		if (errorCode == tinyxml2::XML_SUCCESS)
		{
			value = temp != 0;
		}
		else
		{
			value = {};
		}
	}

	void operator()(const char* name, int& value)
	{
		errorCode = XmlParent->QueryIntAttribute(name, &value);
	}

	void operator()(const char* name, bool& value)
	{
		errorCode = XmlParent->QueryBoolAttribute(name, &value);
	}
*/
#ifdef _MFC_VER

	// MFC types Support.
	template<>
	void operator()(const char* name, CRect& value)
	{
		auto xml = XmlParent->FirstChildElement(name);
		if (xml == nullptr)
			return;

		int temp = 0;

		errorCode = xml->QueryIntAttribute("l", &temp);
		if (errorCode != tinyxml2::XML_SUCCESS)
			return;
		value.left = temp;

		errorCode = xml->QueryIntAttribute("r", &temp);
		if (errorCode != tinyxml2::XML_SUCCESS)
			return;
		value.right = temp;

		errorCode = xml->QueryIntAttribute("t", &temp);
		if (errorCode != tinyxml2::XML_SUCCESS)
			return;
		value.top = temp;

		errorCode = xml->QueryIntAttribute("b", &temp);
		if (errorCode != tinyxml2::XML_SUCCESS)
			return;
		value.bottom = temp;
	}

	template<>
	void operator()(const char* name, CPoint& value)
	{
		auto xml = XmlParent->FirstChildElement(name);
		if (xml == nullptr)
			return;

		int temp = 0;

		errorCode = xml->QueryIntAttribute("x", &temp);
		if (errorCode != tinyxml2::XML_SUCCESS)
			return;
		value.x = temp;

		errorCode = xml->QueryIntAttribute("y", &temp);
		if (errorCode != tinyxml2::XML_SUCCESS)
			return;
		value.y = temp;
	}

	template<>
	void operator()(const char* name, CSize& value)
	{
		auto xml = XmlParent->FirstChildElement(name);
		if (xml == nullptr)
			return;

		int temp = 0;

		errorCode = xml->QueryIntAttribute("cx", &temp);
		if (errorCode != tinyxml2::XML_SUCCESS)
			return;
		value.cx = temp;

		errorCode = xml->QueryIntAttribute("cy", &temp);
		if (errorCode != tinyxml2::XML_SUCCESS)
			return;
		value.cy = temp;
	}
#endif // MFC
};