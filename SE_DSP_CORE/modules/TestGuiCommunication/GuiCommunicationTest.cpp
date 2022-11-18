#include "./GuiCommunicationTest.h"

REGISTER_PLUGIN ( GuiCommunicationTest, L"SE GUI Communication Test" );

GuiCommunicationTest::GuiCommunicationTest( IMpUnknown* host ) : MpBase( host )
,counter_(0)
,interval_(0)
{
	// Register pins.
	initializePin( 0, toGui );
	initializePin( 1, updateRateHz );
}

void GuiCommunicationTest::subProcess( int bufferOffset, int sampleFrames )
{
	counter_ = counter_ - sampleFrames;
	while( counter_ < 0 )
	{
		// send an update to GUI via parameter.
		toGui.setValue( toGui + 1, bufferOffset );

		// send an update to GUI via que.
		getHost()->sendMessageToGui(0, sizeof(counter_), &counter_);

		// reset counter.
		counter_ += interval_;
	}
}

void GuiCommunicationTest::onSetPins(void)
{
	// Check which pins are updated.
	if( updateRateHz.isUpdated() )
	{
		interval_ = (int) ( getSampleRate() / updateRateHz );
	}

	// Set processing method.
	SET_PROCESS( &GuiCommunicationTest::subProcess );

	// Disable automatic sleeping
	setSleep( false );
}

int32_t GuiCommunicationTest::receiveMessageFromGui(int32_t id, int32_t size, const void* messageData)
{
	return gmpi::MP_OK;
}
