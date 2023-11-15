#include "./FileDialogGui.h"

#include "../shared/unicode_conversion.h"
#include "../shared/it_enum_list.h"
#include "../shared/string_utilities.h"
#include "../se_sdk3/MpString.h"

using namespace gmpi;
using namespace gmpi_gui;
using namespace gmpi_sdk;
using namespace JmUnicodeConversions;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, FileDialogGui, L"SE FileDialog" );

FileDialogGui::FileDialogGui() :
m_prev_trigger(false)
{
	// initialise pins.
	initializePin( pinFileName );
	initializePin( pinFileExtension);
	initializePin( pinTrigger, static_cast<MpGuiBaseMemberPtr2>(&FileDialogGui::onSetTrigger) );
	initializePin( pinSaveMode);
}

std::string FileDialogGui::getDefaultFolder(std::wstring extension)
{
	const std::wstring searchFilename = L"dummy." + extension;
	const auto fullFileName = uiHost.resolveFilename(searchFilename.c_str());
	return JmUnicodeConversions::WStringToUtf8(fullFileName.substr(0, fullFileName.find(L"dummy") - 1));
}

void FileDialogGui::onSetTrigger()
{
	// trigger on mouse-up
	if( pinTrigger == false && m_prev_trigger == true ) // dialog triggered on mouse-up (else dialog grabs focus, button never resets)
	{
		std::wstring filename = pinFileName;
		std::wstring file_extension = pinFileExtension;

		IMpGraphicsHostBase* dialogHost = 0;
		getHost()->queryInterface(SE_IID_GRAPHICS_HOST_BASE, reinterpret_cast<void**>( &dialogHost ));

		if( dialogHost != 0 )
		{
			int dialogMode = (int) pinSaveMode;
			dialogHost->createFileDialog(dialogMode, nativeFileDialog.GetAddressOf());

			if( ! nativeFileDialog.isNull() )
			{
				nativeFileDialog.AddExtensionList(pinFileExtension);

				auto filename = pinFileName.getValue();
				if (!filename.empty())
				{
					filename = uiHost.resolveFilename(filename);
					nativeFileDialog.SetInitialFullPath(JmUnicodeConversions::WStringToUtf8(filename));
				}
				else
				{
					nativeFileDialog.setInitialDirectory(getDefaultFolder(pinFileExtension));
				}

				nativeFileDialog.ShowAsync([this](int32_t result) -> void { this->OnFileDialogComplete(result); });
			}
		}
	}

	m_prev_trigger = pinTrigger;
}

void FileDialogGui::OnFileDialogComplete(int32_t result)
{
	if (result == gmpi::MP_OK)
	{
		// Trim filename if in default folder.
		auto filepath = nativeFileDialog.GetSelectedFilename();
		auto fileext = GetExtension(filepath);
		const char* fileclass = nullptr;

		if (fileext == "sf2" || fileext == "sfz")
		{
			fileclass = "Instrument";
		}
		else
		{
			if (fileext == "png" || fileext == "bmp" || fileext == "jpg")
			{
				fileclass = "Image";
			}
			else
			{
				if (fileext == "wav")
				{
					fileclass = "Audio";
				}
				else
				{
					if (fileext == "mid")
					{
						fileclass = "MIDI";
					}
				}
			}
		}

		if (fileclass)
		{
			auto shortName = StripPath(filepath);

			const auto r = uiHost.FindResourceU(shortName.c_str(), fileclass);
			
			if (filepath == r)
			{
				filepath = shortName;
			}
		}

		pinFileName = filepath;
	}

	nativeFileDialog.setNull(); // release it.
}
