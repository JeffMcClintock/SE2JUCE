#include "mp_sdk_gui2.h"
#include "../se_sdk3/mp_gui.h"
#include "TimerManager.h"

using namespace gmpi;

class ConfirmationDialogGui final : public SeGuiInvisibleBase, public TimerClient
{
	void onSetFileName()
	{
		// pinFileName changed
	}

	void onSetFileExtension()
	{
		// pinFileExtension changed
	}

	void onSetTrigger()
	{
		// trigger on mouse-up
		if (pinTrigger == false && m_prev_trigger == true) // dialog triggered on mouse-up (else dialog grabs focus, button never resets)
		{
			std::wstring filename = pinTitle;
			std::wstring file_extension = pinBodytext;

			gmpi_gui::IMpGraphicsHost* dialogHost = 0;
			getHost()->queryInterface(gmpi_gui::SE_IID_GRAPHICS_HOST, reinterpret_cast<void**>(&dialogHost));

			if (dialogHost != 0)
			{
				dialogHost->createOkCancelDialog(0, nativeDialog.GetAddressOf());

				if (!nativeDialog.isNull())
				{
					const auto title = JmUnicodeConversions::WStringToUtf8(pinTitle.getValue());
					const auto text = JmUnicodeConversions::WStringToUtf8(pinBodytext.getValue());
					nativeDialog.SetTitle(title.c_str());
					nativeDialog.SetText(text.c_str());

					nativeDialog.ShowAsync([this](int32_t result) -> void { this->OnDialogComplete(result); });
				}
			}
		}

		m_prev_trigger = pinTrigger;
	}

	void OnDialogComplete(int32_t result)
	{
		if (result == gmpi::MP_OK)
		{
			pinOK = true;
		}
		else
		{
			pinCancel = true;
		}

		nativeDialog.setNull(); // release it.

		StartTimer();
	}

	bool OnTimer() override
	{
		pinOK = false;
		pinCancel = false;
		return false;
	}

	StringGuiPin pinTitle;
	StringGuiPin pinBodytext;
	BoolGuiPin pinTrigger;
	BoolGuiPin pinOK;
	BoolGuiPin pinCancel;

	GmpiGui::OkCancelDialog nativeDialog;
	bool m_prev_trigger{};

public:
	ConfirmationDialogGui()
	{
		initializePin(pinTitle);
		initializePin(pinBodytext);
		initializePin(pinTrigger, static_cast<MpGuiBaseMemberPtr2>(&ConfirmationDialogGui::onSetTrigger));
		initializePin(pinOK);
		initializePin(pinCancel);
	}
};

namespace
{
	auto r = Register<ConfirmationDialogGui>::withId(L"SE Confirmation Dialog");
}
