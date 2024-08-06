#include "Vst2Preset.h"
#include "../tinyXml2/tinyxml2.h"
#include "conversion.h"
#include "vstfxstore.h"
#include <fstream>

using namespace std;

namespace Vst2PresetUtil
{
	void WritePreset(platform_string wfilename, int32_t Vst2Id, std::string presetName, const std::string& xmlPreset)
	{
		fxChunkSet header = {};
		wrapperHeader header2 = {};
		const auto header_size = sizeof(header) - sizeof(char[8]); // minus psudo-member.
		const auto header_size2 = sizeof(header2.sizeA) + sizeof(header2.sizeB) + sizeof(header2.sizeC); // minus psudo-member.

		header.chunkMagic = id_to_long2("CcnK");		// 'CcnK'
		header.chunkSize = header_size2 + xmlPreset.size();
		header.byteSize = header_size - sizeof(header.chunkMagic) - sizeof(header.byteSize) + header_size2 + xmlPreset.size();		// of this chunk, excl. magic + byteSize
		header.fxMagic = id_to_long2("FPCh");
		header.version = 1;
		header.fxID = Vst2Id;				// fx unique id
		header.fxVersion = 1;
		header.numPrograms = 1;
		presetName = presetName.substr(0, (std::min)(presetName.size(), sizeof(header.prgName) - 1));
		strcpy(header.prgName, presetName.c_str());

		ofstream f(wfilename, ios_base::out | ios_base::binary);

		// Correct endian.
		reverse((int*)&header, 7);
		reverse(&(header.chunkSize), 1);

		f.write((char*)&header, header_size); // VST2 header

		header2.sizeA = xmlPreset.size() + sizeof(header2.sizeC); // total size
		header2.sizeC = xmlPreset.size(); // XML size
		f.write((char*)&header2, header_size2);

		f.write(xmlPreset.data(), xmlPreset.size()); // XML string
	}

	// Returns XML payload string from (modern) SE vstpreset.
	std::string ReadPreset(platform_string wfilename, int32_t Vst2Id_32bit, int32_t Vst2Id_64bit)
	{
		ifstream f(wfilename, ios_base::in | ios_base::binary);

		fxChunkSet header;
		const int header_size = sizeof(header) - sizeof(char[8]);
		f.read((char*) &header, header_size);
		reverse((int*)&header, 7);
		reverse(&(header.chunkSize), 1);

		if (header.fxID != Vst2Id_64bit && header.fxID != Vst2Id_32bit)
		{
			return {}; // not compatible
		}

		const int chunk_size = header.chunkSize; // for presets
		std::string chunk;
		chunk.resize(chunk_size);
		f.read((char*) chunk.data(), chunk_size);

		const auto header2 = (wrapperHeader*)chunk.data();

		// Check it's the modern XML format.
		const bool isValidXmlPreset = header2->sizeB == 0 && header2->xml[0] == '<' && header2->xml[1] == '?'&& header2->xml[2] == 'x';
		if(!isValidXmlPreset)
		{
			return {};
		}

		std::string xmlString(header2->xml, header2->sizeC); // ensure null-terminated.

		return xmlString;
	}
}
