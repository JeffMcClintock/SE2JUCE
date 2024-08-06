#include "./GuiCommunicationTestGui.h"

// disable MS warning about sprintf
#pragma warning(disable : 4996)


REGISTER_GUI_PLUGIN( GuiCommunicationTestGui, L"SE GUI Communication Test" );

GuiCommunicationTestGui::GuiCommunicationTestGui(IMpUnknown* host) : SeGuiWindowsGfxBase(host)
,updateCount_(0)
,updateHz_(0.0f)
,paintCount_(0)
,repaintHz_(0.0f)
{
	// initialise pins
	fromDSP.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&GuiCommunicationTestGui::onUpdateFromDsp) );
}

// handle pin updates
void GuiCommunicationTestGui::onUpdateFromDsp()
{
	updateCount_++;

	invalidateRect();
}

int32_t GuiCommunicationTestGui::receiveMessageFromAudio(int32_t id, int32_t size, void* messageData)
{
	return gmpi::MP_OK;
}

int32_t GuiCommunicationTestGui::paint( HDC hDC )
{
	MpRect r1 = getRect();
	int width = r1.right - r1.left;
	int height = r1.bottom - r1.top;

	HBRUSH background_brush = CreateSolidBrush( repaintColour );
	repaintColour += 0x3457252;

	RECT r;
	r.top = r.left = 0;
	r.right = width + 1;
	r.bottom = height + 1;
	FillRect(hDC, &r, background_brush);

	// cleanup objects
	DeleteObject(background_brush);

	wchar_t buf[40];
	swprintf( buf, L"from-DSP %3.1f Hz", updateHz_ );
	TextOut( hDC, 0, 0, buf, (int) wcslen( buf ) );

	swprintf( buf, L"repaint %3.1f Hz", repaintHz_ );
	TextOut( hDC, 0, 20, buf, (int) wcslen( buf ) );

	paintCount_++;

	// send an update to GUI via que.
	getHost()->sendMessageToAudio(0, sizeof(paintCount_), &paintCount_);

	return gmpi::MP_OK;
}

int32_t GuiCommunicationTestGui::onTimer()
{
	updateHz_ = (float)updateCount_;
	updateCount_ = 0;

	repaintHz_ = (float)paintCount_;
	paintCount_ = 0;

	return gmpi::MP_OK;
}


int32_t MP_STDCALL GuiCommunicationTestGui::openWindow( HWND parentWindow, HWND& returnWindowHandle )
{
	MpGuiWindowsGfxBase::openWindow( parentWindow, returnWindowHandle );

	// start a timer to drive anumation.  See onTimer() member.
	timerId_ = SetTimer( getWindowHandle(),
		0,		// timer number.
		1000,		// elapsed time (ms).
		0		// TIMERPROC (not used here).
		);

	return gmpi::MP_OK;
}

int32_t MP_STDCALL GuiCommunicationTestGui::closeWindow( void )
{
	KillTimer( getWindowHandle(), timerId_ );
	return MpGuiWindowsGfxBase::closeWindow();
}