#include "mp_sdk_gui2.h"

using namespace gmpi;

class UserSettingTextGui : public SeGuiInvisibleBase
{
	void onSetvaluein()
	{
		pinValue = pinvaluein;
	}

	void onSetValue()
	{
		pinvaluein = pinValue;
	}

	StringGuiPin pinvaluein;
	StringGuiPin pinValue;

public:
	UserSettingTextGui()
	{
		initializePin(3, pinvaluein, static_cast<MpGuiBaseMemberPtr2>(&UserSettingTextGui::onSetvaluein));
		initializePin(pinValue, static_cast<MpGuiBaseMemberPtr2>(&UserSettingTextGui::onSetValue));
	}
};

namespace
{
	auto r = Register<UserSettingTextGui>::withId(L"SE UserSettingText");
}
SE_DECLARE_INIT_STATIC_FILE(UserSettingText_Gui);