#include "./ConvertersGui.h"

using namespace gmpi;

SE_DECLARE_INIT_STATIC_FILE(Converters_GUI)

typedef SimpleGuiConverter<int, bool> IntToBool;
typedef SimpleGuiConverter<int, float> IntToFloat;
typedef SimpleGuiConverter<int, std::wstring> IntToText;

typedef SimpleGuiConverter<float, int> FloatToInt;
typedef SimpleGuiConverter<float, bool> FloatToBool;
//typedef SimpleGuiConverter<float, std::wstring> FloatToText;

typedef SimpleGuiConverter<bool, int> BoolToInt;
typedef SimpleGuiConverter<bool, float> BoolToFloat;
typedef SimpleGuiConverter<bool, std::wstring> BoolToText;

typedef SimpleGuiConverter<std::wstring, int> TextToInt;
typedef SimpleGuiConverter<std::wstring, float> TextToFloat;
typedef SimpleGuiConverter<std::wstring, bool> TextToBool;

typedef SimpleGuiConverter<std::wstring, MpBlob> TextToBlob;
typedef SimpleGuiConverter<MpBlob, std::wstring> BlobToText;

REGISTER_GUI_PLUGIN( IntToBool, L"SE IntToBool GUI" );
REGISTER_GUI_PLUGIN( IntToFloat, L"SE IntToFloat GUI" );
REGISTER_GUI_PLUGIN( IntToText, L"SE IntToText GUI" );

REGISTER_GUI_PLUGIN( FloatToInt, L"SE FloatToInt GUI" );
REGISTER_GUI_PLUGIN( FloatToBool, L"SE FloatToBool GUI" );
//REGISTER_GUI_PLUGIN( FloatToText, L"SE FloatToText GUI" );

REGISTER_GUI_PLUGIN( BoolToInt, L"SE BoolToInt GUI" );
REGISTER_GUI_PLUGIN( BoolToFloat, L"SE BoolToFloat GUI" );
REGISTER_GUI_PLUGIN( BoolToText, L"SE BoolToText GUI" );

REGISTER_GUI_PLUGIN( TextToInt, L"SE TextToInt GUI" );
REGISTER_GUI_PLUGIN( TextToFloat, L"SE TextToFloat GUI" );
REGISTER_GUI_PLUGIN( TextToBool, L"SE TextToBool GUI" );

REGISTER_GUI_PLUGIN( TextToBlob, L"SE TextToBlob GUI" );
REGISTER_GUI_PLUGIN( BlobToText, L"SE BlobToText GUI" );

class PasswordHideGui : public SeGuiInvisibleBase
{
	std::wstring obfuscated(std::wstring password)
	{
		std::wstring result;
		result.assign(password.size(), L'*');
		return result;
	}

	void onSetInput()
	{
		pinOutput = obfuscated(pinInput.getValue());
	}

	void onSetOutput()
	{
		if (pinOutput.getValue() != obfuscated(pinInput.getValue()))
		{
			pinInput = pinOutput;
		}
	}

	StringGuiPin pinInput;
	StringGuiPin pinOutput;
public:

	PasswordHideGui()
	{
		initializePin(pinInput, static_cast<MpGuiBaseMemberPtr2>(&PasswordHideGui::onSetInput));
		initializePin(pinOutput, static_cast<MpGuiBaseMemberPtr2>(&PasswordHideGui::onSetOutput));
	}
};

namespace
{
	auto r = Register<PasswordHideGui>::withId(L"SE Password Hide");
}