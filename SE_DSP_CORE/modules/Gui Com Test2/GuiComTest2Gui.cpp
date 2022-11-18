#include "mp_sdk_gui2.h"

using namespace gmpi;

class Guicomtest2Gui : public gmpi_gui::MpGuiGfxBase
{
	int clickCount = 0;
public:
	Guicomtest2Gui()
	{
	}

	int32_t MP_STDCALL onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		setCapture();

		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		releaseCapture();

		std::vector<char> data;

		if ((++clickCount % 3) == 0)
		{
			// 6MB
			data.assign(1048576 * 6, 0);
		}
		else
		{
			data.assign(23, 0);
		}

		getHost()->sendMessageToAudio( 23, data.size(), data.data() );

		return gmpi::MP_OK;
	}

	int32_t measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override
	{
		const float minSize = 15;

		returnDesiredSize->width = (std::max)(minSize, availableSize.width);
		returnDesiredSize->height = (std::max)(minSize, availableSize.height);

		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL receiveMessageFromAudio( int32_t id, int32_t size, const void* messageData ) override
	{
		_RPT1(_CRT_WARN, "GUI: recieveMessageFromDsp size %d bytes\n", size);

		return gmpi::MP_OK;
	}
};

namespace
{
	auto r = Register<Guicomtest2Gui>::withId(L"SE GUI COM Test2");
}
