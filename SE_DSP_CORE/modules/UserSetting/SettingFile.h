#pragma once
#include <string>
#include <algorithm>
#include "../tinyXml2/tinyxml2.h"
#ifdef _WIN32
#include "shlobj.h"
#else
#include <CoreFoundation/CoreFoundation.h>
#include <pwd.h>
#endif
#include "../shared/unicode_conversion2.h"
#include <sys/stat.h>

using namespace FastUnicode;
using namespace tinyxml2;

namespace SettingsFile
{
	// Determine settings file: C:\Users\Jeff\AppData\Local\Plugin\Preferences.xml
	inline std::string getSettingFilePath(std::wstring product)
	{
#ifdef _WIN32
		wchar_t mySettingsPath[MAX_PATH];
		SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, mySettingsPath);
		std::wstring meSettingsFile(mySettingsPath);
		meSettingsFile += L"/";
#else
        const char* homeDir = getenv("HOME");
        
        if(!homeDir)
        {
            struct passwd* pwd = getpwuid(getuid());
            if (pwd)
                homeDir = pwd->pw_dir;
        }
        
        auto meSettingsFile = Utf8ToWstring(homeDir) + L"/Library/Preferences/";
#endif
		meSettingsFile += product;

		// Create folder if not already.
#ifdef _WIN32
		_wmkdir(meSettingsFile.c_str());
#else
		mkdir(WStringToUtf8(meSettingsFile).c_str(), 0775);
#endif

		meSettingsFile += L"/Preferences.xml";

		return  WStringToUtf8(meSettingsFile);
	}

	inline std::wstring Sanitize(std::wstring s)
	{
		wchar_t remove[] = { L'!', L'#', L'$', L'%', L'&', L'(', L')', L'*', L'+', L',', L'.', L'/', L':', L';', L'<', L'=', L'>', L'?', L'@', L'[', L'\\', L']', L'^', L'{', L'|', L'}', L'~' };

		for (auto c : remove)
		{
			s.erase(std::remove(s.begin(), s.end(), c), s.end());
		}

		// replace with underscore.
		wchar_t replace[] = { L' ', L'-' };

		for (auto c : replace)
		{
			std::replace(s.begin(), s.end(), c, L'_');
		}

		return s;
	}

	inline std::wstring GetValue(std::wstring product, std::wstring key, std::wstring pdefault)
	{
		key = Sanitize(key);

		tinyxml2::XMLDocument doc;

        if (XML_SUCCESS == doc.LoadFile(getSettingFilePath(product).c_str()) && !doc.Error())
		{
			auto settingsXml = doc.FirstChildElement("Preferences");

			if (settingsXml)
			{
				auto keyXml = settingsXml->FirstChildElement(WStringToUtf8(key).c_str());

				if (keyXml)
				{
					auto s = keyXml->GetText();
					return s ? Utf8ToWstring(s) : L"";
				}
			}
		}

		return pdefault;
	}

	inline void SetValue(std::wstring product, std::wstring pkey, std::wstring value)
	{
		pkey = Sanitize(pkey);

		// Load Settings XML document
		const auto settingFilename = getSettingFilePath(product);
		tinyxml2::XMLDocument doc;

		// If document don't exist, create it.
		XMLElement* element_preferences = nullptr;
		if (XML_SUCCESS != doc.LoadFile(settingFilename.c_str()) || doc.Error())
		{
			doc.Clear();
			doc.ClearError();

			doc.LinkEndChild(doc.NewDeclaration());
		}
		else
		{
			element_preferences = doc.FirstChildElement("Preferences");
		}

		// If preferences element don't exist, create it.
		if (!element_preferences)
		{
			element_preferences = doc.NewElement("Preferences");
			doc.LinkEndChild(element_preferences);
		}

		auto key = WStringToUtf8(pkey);

		auto keyXml = element_preferences->FirstChildElement(key.c_str());

		// If key element don't exist, create it.
		if (!keyXml)
		{
			keyXml = doc.NewElement(key.c_str());
			element_preferences->LinkEndChild(keyXml);
		}

		// If key value element don't exist, create it.
		auto textValue = WStringToUtf8(value);
		XMLText* textE = nullptr;

		if (keyXml->FirstChild())
			textE = keyXml->FirstChild()->ToText();

		if (!textE)
		{
			textE = doc.NewText(textValue.c_str());
			keyXml->LinkEndChild(textE);
		}
		else
		{
			textE->SetValue(textValue.c_str());
		}

		doc.SaveFile(settingFilename.c_str());
	}
}
