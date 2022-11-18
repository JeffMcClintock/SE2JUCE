// prevent MS CPP - 'swprintf' was declared deprecated warning
#define _CRT_SECURE_NO_DEPRECATE 
#pragma warning(disable : 4996)

#include "QueLoaderGui.h"

REGISTER_GUI_PLUGIN( QueLoader_gui, L"SynthEdit QueLoader" );

QueLoader_gui::QueLoader_gui(IMpUnknown* host) : SeGuiWindowsGfxBase(host)
,messageDone(false)
{
	initializePin( 0, pinSamples, static_cast<MpGuiBaseMemberIndexedPtr>(&QueLoader_gui::onValueChanged) );
	initializePin( 1, pinToDsp );
	initializePin( 2, pinRate );
}

void QueLoader_gui::onValueChanged(int index)
{
//	getHost()->sendMessageToAudio( 33, 5, "54321" );

	// loop data back to DSP
//	pinToDsp.setValueRaw( pinSamples.rawSize(0), pinSamples.rawData(0) );

//	pinToDsp.sendPinUpdate();

//	invalidateRect();
}

int32_t QueLoader_gui::openWindow( HWND parentWindow, HWND& returnWindowHandle )
{
	MpGuiWindowsGfxBase::openWindow( parentWindow, returnWindowHandle );

	// start a timer to drive anumation.  See onTimer() member.
	timerId_ = SetTimer( getWindowHandle(),
		0,		// timer number.
		50,		// elapsed time (ms).
		0		// TIMERPROC (not used here).
		);

	return gmpi::MP_OK;
}

int32_t QueLoader_gui::closeWindow( void )
{
	KillTimer( getWindowHandle(), timerId_ );
	return MpGuiWindowsGfxBase::closeWindow();
}

int32_t QueLoader_gui::onTimer()
{
	int target = (int) ( pinRate * 5000.0f );
	int sent = 0;

	while( sent < target )
	{
		// send random message to Audio.
		const int maxSize = 1000;
		unsigned char buffer[maxSize];

		int size = rand() % maxSize;
		if( size > (target - sent) )
			size = (target - sent);

		unsigned char c = size;

		for( int i = 0 ; i < size ; ++i )
		{
			buffer[i] = c;
			++c;
		}

		getHost()->sendMessageToAudio( 123, size, buffer );

		sent += size;
	}

	return gmpi::MP_OK;
}

int32_t QueLoader_gui::receiveMessageFromAudio( int32_t id, int32_t size, void* messageData )
{
	if( messageDone )
		 return gmpi::MP_OK;

	unsigned char* buffer = (unsigned char*) messageData;

	// check for corruption;
	unsigned char c = *buffer;
	if( id == 1234 )
	{
		for( int i = 0 ; i < size ; ++i )
		{
			if( buffer[i] != c )
			{
				messageDone = true;
				MessageBoxA(0,"DSP->GUI Que corruption!!!", "debug msg", MB_OK );
				break;
			}
			++c;
		}
	}

	return gmpi::MP_OK;
};


int32_t QueLoader_gui::paint(HDC hDC)
{
	return gmpi::MP_OK;
}

int32_t QueLoader_gui::measure(MpSize availableSize, MpSize &returnDesiredSize)
{
	const int prefferedSize = 100;
	const int minSize = 15;

	returnDesiredSize.x = availableSize.x;
	returnDesiredSize.y = availableSize.y;
	if(returnDesiredSize.x > prefferedSize)
	{
		returnDesiredSize.x = prefferedSize;
	}
	else
	{
		if(returnDesiredSize.x < minSize)
		{
			returnDesiredSize.x = minSize;
		}
	}
	if(returnDesiredSize.y > prefferedSize)
	{
		returnDesiredSize.y = prefferedSize;
	}
	else
	{
		if(returnDesiredSize.y < minSize)
		{
			returnDesiredSize.y = minSize;
		}
	}
	return gmpi::MP_OK;
};