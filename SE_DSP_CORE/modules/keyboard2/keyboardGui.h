#ifndef Keyboard_gui_H_INCLUDED
#define Keyboard_gui_H_INCLUDED

#include "mp_sdk_gui.h"

#define MAX_KEYS 128

class KeyboardGui :
	public SeGuiCompositedGfxBase
{
public:
	KeyboardGui(IMpUnknown* host);

	// overrides
	virtual int32_t MP_STDCALL paint( HDC hDC );
	virtual int32_t MP_STDCALL onLButtonDown( UINT flags, POINT point );
	virtual int32_t MP_STDCALL onLButtonUp( UINT flags, POINT point );
	virtual int32_t MP_STDCALL onMouseMove( UINT flags, POINT point );
	virtual void getWindowStyles( DWORD& style, DWORD& extendedStyle );

	virtual int32_t MP_STDCALL measure( MpSize availableSize, MpSize &returnDesiredSize );

	FloatArrayGuiPin pinGates;
	FloatArrayGuiPin pinTriggers;
//	FloatArrayGuiPin pinPitches;
	FloatArrayGuiPin pinVelocities;

private:
	void onValueChanged( int index );

	void PlayNote( int p_note_num, bool note_on );
	int PointToKeyNum( POINT &point );
	bool getKeyState( int voice );

	MpFontInfo fontInfo;
	bool m_inhibit_update;

	int midiNote_;
	bool toggleMode_;
	int baseKey_; // MIDI key num of leftmost key.
};
#endif