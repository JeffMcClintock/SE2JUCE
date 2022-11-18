#include "../se_sdk3/mp_sdk_audio.h"

using namespace gmpi;

class UserSettingText : public MpBase2
{
	StringInPin pinvaluein;
	StringOutPin pinValue;

public:
	UserSettingText()
	{
		initializePin(pinvaluein);
		initializePin(pinValue);
	}

	void onSetPins() override
	{
		if (pinvaluein.isUpdated())
		{
			pinValue = pinvaluein;
		}
	}
};

namespace
{
	auto r = Register<UserSettingText>::withId(L"SE UserSettingText");
}
SE_DECLARE_INIT_STATIC_FILE(UserSettingText);