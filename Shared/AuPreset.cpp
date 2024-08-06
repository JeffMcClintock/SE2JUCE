#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "AuPreset.h"
#include "conversion.h"
#include "../tinyXml2/tinyxml2.h"

using namespace std;

namespace AuPresetUtil
{
	bool WritePreset(
        std::wstring wfilename,
        const std::string& presetName,
        std::string plugin_type,
        std::string unique_manufacturer_id,
        std::string vst_4_Char_id,
        std::string xmlPreset
    )
	{
		// XML inside <text> needs some special characters escaped [ <, >, & ]
		// so it can be imbedded in apple's XML.
		std::vector< std::pair< std::string, std::string > > replaceStrings =
		{
			{"&", "&amp;"},
			{"<", "&lt;"},
			{">", "&gt;"},
		//	{"&quot;", "\"" },
		//	{"&apos;", "\'"},
		};

		// escape some special characters.
		for (auto& rs : replaceStrings)
		{
			auto p = xmlPreset.find(rs.first);

			while (p != string::npos)
			{
				xmlPreset = xmlPreset.substr(0, p) + rs.second + xmlPreset.substr(p + rs.first.size(), xmlPreset.size() - p - rs.first.size());
				p += rs.second.size() + 1;
				p = xmlPreset.find(rs.first, p);
			}
		}

		// Convert ASCII 4-char code2 to 32-bit integer.
        auto pluginType = idToInt32(plugin_type);
        auto numericalId = idToInt32(vst_4_Char_id);
		auto numericalManufacturerId = idToInt32(unique_manufacturer_id);

		std::stringstream oss;
		oss <<
R"RAW(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>SEPRESET</key>
	<string>)RAW" << xmlPreset << R"RAW(</string>
	<key>data</key>
	<data>
	AAAAAAAAAAAAAAAA
	</data>
	<key>manufacturer</key>
	<integer>)RAW" << numericalManufacturerId << R"RAW(</integer>
	<key>name</key>
	<string>)RAW" << presetName << R"RAW(</string>
	<key>subtype</key>
	<integer>)RAW" << numericalId << R"RAW(</integer>
	<key>type</key>
	<integer>)RAW" << pluginType << R"RAW(</integer>
	<key>version</key>
	<integer>0</integer>
</dict>
</plist>)RAW";

#ifdef _WIN32
		FILE* f1 = nullptr;
		_wfopen_s(&f1, wfilename.c_str(), L"w");
#else
        FILE* f1 = fopen(WStringToUtf8(wfilename).c_str(), "w");
#endif
		if (nullptr == f1)
		{
			return false; // use _strerror_s() to get error message.
		}

		fputs(oss.str().c_str(), f1);
		fclose(f1);
		return true;
	}

	// Returns XML payload string from aupreset.
	std::string ReadPreset(std::wstring wfilename)
	{
        tinyxml2::XMLDocument presetDoc;

		presetDoc.LoadFile( WStringToUtf8(wfilename).c_str() );

		if (!presetDoc.Error())
		{
			auto plist_E = presetDoc.FirstChildElement("plist");
			if (!plist_E) return "";

			auto dict_E = plist_E->FirstChildElement("dict");
			if (!dict_E) return "";

			std::string key;

			for (auto e = dict_E->FirstChildElement(); e; e = e->NextSiblingElement())
			{
				if (strcmp(e->Value(), "key") == 0)
				{
					key = e->GetText();
				}
				else if (key == "SEPRESET" && strcmp(e->Value(), "string") == 0)
                {
                    return e->GetText();
                }
			}
		}

		return {};
	}
}
