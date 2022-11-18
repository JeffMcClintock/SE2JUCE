#include "../se_sdk3/mp_sdk_audio.h"
#ifdef _WIN32
#include <crtdbg.h>
#endif

using namespace gmpi;

class IdeLogger final : public MpBase2
{
	StringInPin pinTextVal;

public:
	IdeLogger()
	{
		initializePin( pinTextVal );
	}

	void onSetPins() override
	{
		// Check which pins are updated.
		if( pinTextVal.isUpdated() )
		{
		    #if defined(_WIN32) && defined (_DEBUG)
			const auto* filename = L""; // !!! these could also be pins, to customize message !!!
			const auto* moduleName = L"";
			const int linenumber = {};
			_CrtDbgReportW(0, filename, linenumber, moduleName, L"%s\n", pinTextVal.getValue().c_str());
			#endif
		}
	}
};

namespace
{
	auto r = Register<IdeLogger>::withId(L"SE IDE Logger");
}
SE_DECLARE_INIT_STATIC_FILE(IdeLogger);
