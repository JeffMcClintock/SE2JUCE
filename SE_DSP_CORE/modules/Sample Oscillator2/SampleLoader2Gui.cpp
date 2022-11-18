#include <sstream> 
#include "SampleLoader2Gui.h"
#include "csoundfont.h"
#include "RiffFile2.h"
#include "../shared/unicode_conversion.h"

REGISTER_GUI_PLUGIN( SampleLoader2Gui, L"SE Sample Loader2Gui" );

SampleLoader2Gui::SampleLoader2Gui(IMpUnknown* host) : MpGuiBase(host)
{
	initializePin( 0, pinFilename, static_cast<MpGuiBaseMemberPtr>( &SampleLoader2Gui::onFileNameChanged ) );
	initializePin( 1, pinBank, static_cast<MpGuiBaseMemberPtr>( &SampleLoader2Gui::onFileNameChanged ) );
	initializePin( 2, pinBankNames );
	initializePin( 3, pinPatchNames );
	initializePin( 4, pinPatch );
}

void SampleLoader2Gui::onFileNameChanged() // or Bank.
{
	RiffFile2 riff;
	uint32_t riff_type;

	wchar_t fullFileName[500];
	getHost()->resolveFilename(pinFilename.getValue().c_str(), (int32_t) sizeof(fullFileName)/sizeof(fullFileName[0]), &fullFileName[0]);
	const auto fullFileNameU = JmUnicodeConversions::WStringToUtf8(fullFileName);

	gmpi_sdk::mp_shared_ptr<gmpi::IMpUserInterfaceHost2> host2;
	getHost()->queryInterface(gmpi::MP_IID_UI_HOST2, host2.asIMpUnknownPtr());

	assert(host2); // new way

	gmpi_sdk::mp_shared_ptr<gmpi::IProtectedFile2> stream2;
	host2->OpenUri(fullFileNameU.c_str(), stream2.getAddressOf());
	if (!stream2)
	{
		pinPatchNames = L"<none>";
		pinBankNames = L"<none>";
		return;
	}
/*
	// open imbedded (or not) file.
	gmpi::IProtectedFile* file = 0;

	int r = getHost()->open ProtectedFile( pinFilename.getValue().c_str(), &file );

	if( r != gmpi::MP_OK )
	{
		pinPatchNames = L"<none>";
		pinBankNames = L"<none>";
		return;
	}
*/
	riff.Open( stream2.get(), riff_type);

	// Create list of chunks we could use
	if( riff_type != MAKEFOURCC('s', 'f', 'b', 'k') )	// sf2 file
	{
//		file->close();
		return;
	}

	unsigned int count_phdr;
	sfPresetHeader* chunk_phdr = 0;

	riff.REG_CHUNK( "Preset Headers", "pdta", "phdr", &chunk_phdr, &count_phdr, sfPresetHeader );

	riff.ReadFile();

//	file->close();

	std::string presetNames;

	std::map<int,std::string> presets;
	std::map<int,int> banks;

	if( chunk_phdr == 0 ) // file not loaded or corrupt
	{
		return;
	}

	sfPresetHeader* last_phdr = chunk_phdr + count_phdr - 1;

	// collect bank numbers, duplicates will fail to insert.
	for( sfPresetHeader* phdr = chunk_phdr ; phdr < last_phdr ; ++phdr )
	{
		banks.insert( std::pair<int,int>( phdr->wBank, 0) );
	}

	// collect patch numbers for current bank.
	int currentBank = pinBank;
	{
		auto it = banks.find(pinBank);
		if (it == banks.end())
		{
			if (banks.empty())
			{
				currentBank = 0;
			}
			else
			{
				currentBank = banks.begin()->first; // choose first availalbe bank.
			}
		}
	}

	for( sfPresetHeader* phdr = chunk_phdr ; phdr < last_phdr ; ++phdr )
	{
		if( currentBank == phdr->wBank )
		{
			presets.insert( std::pair<int,std::string>( phdr->wPreset, phdr->achPresetName) );
		}
	}
/* moved
	// Ensure patch number is valid for this soundfont.
	std::map<int,std::string>::iterator it2 = presets.find(pinPatch);
	if( it2 == presets.end() )
	{
		pinPatch = presets.begin()->first; // choose first availalbe preset.
	}
*/

	delete [] (char*) chunk_phdr;

	// Convert patch names to wide-string.
	std::wstring presetNamesWideChar;

	if( presets.size() != 0 )
	{
		presetNames.clear();
		std::ostringstream os;

		int index = 0;
		for( std::map<int,std::string>::iterator it = presets.begin() ; it != presets.end() ; ++it )
		{
			if( index > 0 )
			{
				os << ',';
			}
			
			// sprintf( presetName, "%3d %s", (*it).first, (*it).second.c_str() );
			os << (*it).first << ' ' << (*it).second.c_str();

			// Add index if needed e.g. ",mypatch=23".
			if( (*it).first != index )
			{
				index = (*it).first;
				//sprintf( presetName, "=%d", index );
				//presetNames.append( presetName );
				os << '=' << index;
			}

			++index;
		}
	 
		presetNames = os.str();

		presetNamesWideChar.clear();
		presetNamesWideChar.assign( presetNames.length(), L' ' );
		std::copy( presetNames.begin(), presetNames.end(), presetNamesWideChar.begin() );
		pinPatchNames = presetNamesWideChar;
	}
	else
	{
		pinPatchNames = L"";
	}

	// convert bank numbers to string.
	if( banks.size() != 0 )
	{
		presetNames.clear();
		std::ostringstream os;

		int index = 0;
		for( std::map<int,int>::iterator it = banks.begin() ; it != banks.end() ; ++it )
		{
			if( index > 0 )
			{
				os << ',';
			}
			
			// sprintf( presetName, "%d", (*it).first );
			os << (*it).first;

			// Add index if needed e.g. ",mypatch=23".
			if( (*it).first != index )
			{
				index = (*it).first;
				//sprintf( presetName, "=%d", index );
				//presetNames.append( presetName );
				os << '=' << index;
			}

			++index;
		}

		presetNames = os.str();
		presetNamesWideChar.clear();
		presetNamesWideChar.assign( presetNames.length(), L' ' );
		std::copy( presetNames.begin(), presetNames.end(), presetNamesWideChar.begin() );
		pinBankNames = presetNamesWideChar;
	}
	else
	{
		pinBankNames = L"";
	}

	// Ensure bank number is valid for this soundfont.
	pinBank = currentBank;

	// Ensure patch number is valid for this soundfont.
	{
		auto it2 = presets.find(pinPatch);
		if (it2 == presets.end())
		{
			if (!presets.empty())
			{
				pinPatch = presets.begin()->first; // choose first availalbe preset.
			}
			else
			{
				pinPatch = 0;
			}
		}
	}
}
