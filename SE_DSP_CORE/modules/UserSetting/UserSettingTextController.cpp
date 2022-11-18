#include "../se_sdk3/mp_sdk_common.h" 
#include <array>
#include <string>
#include "SettingFile.h"
#include "mp_sdk_gui2.h"

using namespace gmpi;

class UserSettingTextController : public GmpiSdk::Controller
{
	std::array< std::wstring, 3 > pinDefaults;
	std::array< bool, 3 > pinDefaultSetFlag;
	int32_t handle = -1;
	bool init = false;

public:

	// Establish connection to host.
	virtual int32_t MP_STDCALL setHost(gmpi::IMpUnknown* phost) override
	{
		auto r = GmpiSdk::Controller::setHost(phost);

		getHost()->getHandle(handle);

		return r;
	}

	// Pins defaults.
	virtual int32_t MP_STDCALL setPinDefault(int32_t pinType, int32_t pinId, const char* defaultValue) override
	{
		// Store Product, Key, and Default-value pin defaults.
		if (pinType == 1 && pinId < 3)
		{
			pinDefaults[pinId] = Utf8ToWstring(defaultValue);
			pinDefaultSetFlag[pinId] = true;
		}

		// Once all needed pins are initialised, retrieve registry setting into parameter.
		if (pinDefaultSetFlag[0] && pinDefaultSetFlag[1] && pinDefaultSetFlag[2])
		{
			auto val = SettingsFile::GetValue(pinDefaults[0], pinDefaults[1], pinDefaults[2]); // Product, Key, Default.

			const int parameterId = 0;
			const int voiceId = 0;
			getHost()->setParameter(getHost()->getParameterHandle(handle, parameterId), MP_FT_VALUE, voiceId, (const void*)val.data(), (int32_t)val.size() * sizeof(val[0]));
			init = true;
		}

		return gmpi::MP_OK;
	}

	// IMpParameterObserver
	virtual int32_t MP_STDCALL setParameter(int32_t parameterHandle, int32_t fieldId, int32_t voice, const void* data, int32_t size) override
	{
		if (fieldId == MP_FT_VALUE && init)
		{
			std::wstring val((wchar_t*)data, size / sizeof(wchar_t));
			SettingsFile::SetValue(pinDefaults[0], pinDefaults[1], val);
		}

		return gmpi::MP_OK;
	}

	UserSettingTextController()
	{
		pinDefaultSetFlag.fill(false);
	}
};

namespace
{
	auto r = Register<UserSettingTextController>::withId(L"SE UserSettingText");
}
SE_DECLARE_INIT_STATIC_FILE(UserSettingText_Controller);