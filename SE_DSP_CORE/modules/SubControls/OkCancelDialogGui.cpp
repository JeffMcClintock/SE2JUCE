#include "mp_sdk_gui2.h"
#include "../se_sdk3/mp_gui.h"

using namespace gmpi;

#if 0 //TODO: expose OK/Cancel dialog

<Plugin id = "SE OK Cancel Dialog" name = "OK Cancel Dialog" category = "Sub-Controls" >
<GUI>
<Pin name = "Heading" datatype = "string" />
<Pin name = "Text" datatype = "string" />
<Pin name = "Trigger" datatype = "bool" direction = "out" />
</GUI>
</Plugin>

class OkCancelDialogGui : public gmpi_gui::MpGuiGfxBase //SeGuiInvisibleBase
{
 	void onSetTrigger()
	{
		GmpiGui::GraphicsHost host(getGuiHost());
		nativeDialog = host.createPlatformDialog();

		//nativeFileDialog.AddExtensionList(pinExtension);
		//nativeFileDialog.SetInitialFullPath(pinpatchValue);

		nativeDialog.ShowAsync([this](int32_t result) -> void { this->OnDialogComplete(result); });
	}

	void OnDialogComplete(int32_t result)
	{
		if (result == gmpi::MP_OK)
		{
//			OnWidgetUpdate(nativeFileDialog.GetSelectedFilename());
		}

		nativeDialog.setNull(); // release it.
	}

 	StringGuiPin pinHeading;
 	StringGuiPin pinText;
 	BoolGuiPin pinTrigger;

public:
	OkCancelDialogGui()
	{
		initializePin( pinHeading );
		initializePin( pinText );
		initializePin( pinTrigger, static_cast<MpGuiBaseMemberPtr2>(&OkCancelDialogGui::onSetTrigger) );
	}
};

namespace
{
	auto r = Register<OkCancelDialogGui>::withId(L"SE OK Cancel Dialog");
}
#endif