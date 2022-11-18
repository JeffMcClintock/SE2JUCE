#include ".\GuiComTest2.h"

REGISTER_PLUGIN ( GuiComTest2, L"SE GUI COM Test2" );

GuiComTest2::GuiComTest2( IMpUnknown* host ) : MpBase( host )
{
	// Register pins.
	initializePin( 0, pinIn );
	initializePin( 1, pinOut );
}

void GuiComTest2::onSetPins(void)
{
	pinOut.setStreaming( pinIn.isStreaming() );
}

int32_t GuiComTest2::receiveMessageFromGui( int32_t id, int32_t size, const void* messageData )
{
	_RPT1(_CRT_WARN, "DSP: recieveMessageFromGui size %d bytes\n", size );

	std::vector<char> data;

	if ((++clickCount % 3) == 0)
	{
		// 6MB
		data.assign(1048576 * 6, 0);
	}
	else
	{
		data.assign(21, 0);
	}

	_RPT1(_CRT_WARN, "DSP: ...sending size %d bytes\n", (int) data.size() );

	getHost()->sendMessageToGui( 23, data.size(), data.data() );

	return gmpi::MP_OK;
}
