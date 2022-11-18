#include "KeyboardGui.h"
#include <gdiplus.h>

using namespace Gdiplus;

REGISTER_GUI_PLUGIN( KeyboardGui, L"SynthEdit Keyboard2" );

KeyboardGui::KeyboardGui(IMpUnknown* host) : SeGuiCompositedGfxBase(host)
,m_inhibit_update(false)
,toggleMode_(false)
{
//	initializePin( 0, pinPitches );
	initializePin( 2, pinGates, static_cast<MpGuiBaseMemberIndexedPtr>(&KeyboardGui::onValueChanged) );
	initializePin( 3, pinVelocities );
}

void KeyboardGui::getWindowStyles( DWORD& style, DWORD& extendedStyle )
{
	style = WS_CHILD | WS_VISIBLE;
	// NOTE: WS_EX_COMPOSITED only availalble on XP or later, and you need to 
	//       define preprocessor constants.. _WIN32_WINNT=0X501;WINVER=0X501
	//       and does not work with GDI+
	//	 extendedStyle = WS_EX_COMPOSITED; // flicker-free redraw.
}

bool KeyboardGui::getKeyState(int voice)
{
	return pinGates.getValue(voice) > 0.f;
}

int KeyboardGui::PointToKeyNum(POINT &point)
{
	MpRect r = getRect();
	int keyHeight = r.bottom - r.top;

	int keyWidth = keyHeight / 5;

	// first assume mouse over white key
	int slot = 2 * (point.x / keyWidth);

	int octave = slot / 14;
	int key = slot % 14;

	if( key > 4 )
		key--;
	if( key > 11 )
		key--;

	// check nearest black key
	int blackKeyHeight = (2*keyHeight + 1)/3;
	int blackKeyWidth = (2*keyWidth + 1) / 3;
	if( point.y <= blackKeyHeight )
	{
		int blackKeyOffset = 0;

		int nearestBlackSlot = (point.x + keyWidth/2) / keyWidth;
		switch(nearestBlackSlot % 7)
		{
		case 0: // slots empty of black keys.
		case 3:
			blackKeyOffset = -1000; // force white key.
			break;
		case 1: // black keys offset left.
		case 4:
			blackKeyOffset = -1;
			break;
		case 2: // black keys offset right.
		case 6:
			blackKeyOffset = 1;
			break;
		};

		int left = nearestBlackSlot * keyWidth - blackKeyWidth/2;
		left += blackKeyOffset * (blackKeyWidth+2) / 6;
		int right = left + blackKeyWidth;

//		_RPT2(_CRT_WARN, "mouse (%d, %d)\n", point.x,point.y );
//		_RPT3(_CRT_WARN, "offset %d, x:%d<-->%d\n", blackKeyOffset,left,right );

		if( point.x > left && point.x <= right )
		{
			octave = nearestBlackSlot / 7;
			key = nearestBlackSlot % 7;

			key = 1 + (key - 1) * 2;

			if( key > 5 )
				key--;
			if( key > 10 )
				key--;
		}
	}

	int note = baseKey_ + octave * 12 + key;

	if( note < 0 )
		note = 0;
	if( note > 127 )
		note = 127;

	return note;
}

int32_t KeyboardGui::paint(HDC hDC)
{
	// determin dimensions.
	MpRect r = getRect();
	int width = r.right - r.left;
	int keyHeight = r.bottom - r.top;

	// WS_EX_COMPOSITED does not work with GDI+, instead...
	// Create a bitmap for double-buffered drawing (flicker-free).
	Bitmap bmp( width, keyHeight );

	// Create GDI+ Graphics object to draw on buffer.
	Graphics* graphics = Graphics::FromImage( &bmp );

	graphics->SetSmoothingMode(SmoothingModeAntiAlias);

	Point pnts[8];

	bool prevWhiteKeyState = false;
	SolidBrush brushShadow(Color::SlateGray);
	const BYTE transparency = 255;
	Pen highlightPen( Color(transparency, 255, 255, 255), 1.f); // white (highlight)

	int keyWidth = keyHeight / 5;

	// don't draw partialy visible keys.
	const int maxWhiteKeys = 75;
	int visibleWhiteKeyCount = width / keyWidth;
	visibleWhiteKeyCount = min( visibleWhiteKeyCount, maxWhiteKeys );

	baseKey_ = 36; // C2 (MIDI Range is C-1 -> G9)
	int baseWhiteKeyNum = 21;

	if( visibleWhiteKeyCount + baseWhiteKeyNum > maxWhiteKeys )
	{
		int extraOctaves = (6 + visibleWhiteKeyCount + baseWhiteKeyNum - maxWhiteKeys ) / 7;
		baseKey_ -= extraOctaves * 12;
	}

	int usableWidth = visibleWhiteKeyCount * keyWidth;

	SolidBrush blueBrush(Color(255,0,0,255));
	graphics->FillRectangle( &blueBrush, usableWidth, 0, width-usableWidth, keyHeight );

	// light-grey white-key fill
	Color keyColor(255, 220, 220, 220);
	Color keyGRadColor(255, 200, 200, 200);
	SolidBrush wbrush(keyColor);
	graphics->FillRectangle( &wbrush, 0, 0, usableWidth, keyHeight );

	// top edge shadow.
	int top_shadow = keyHeight / 16;
	PointF grad1(0.f, -1.f);
	PointF grad2(0.f, (float)top_shadow);
	LinearGradientBrush gradientBrush2( grad1, grad2, Color::SlateGray, keyColor );
	graphics->FillRectangle( &gradientBrush2, 0.0f, 0.0f, (float) usableWidth, (float)top_shadow - 1.0f );

	// white keys highlights + shadows on left
	grad1 = PointF(0.f, (float)keyHeight);
	grad2 = PointF(0.f,(float)keyHeight/2);
	LinearGradientBrush wkgradientBrush( grad1, grad2, keyGRadColor, keyColor );

	int key = baseKey_;
	for( int x = 0 ; x < usableWidth ; x += keyWidth )
	{
		if( getKeyState(key) )
		{
			// gradient on white key
			graphics->FillRectangle( &wkgradientBrush, x, keyHeight/2 + 1, keyWidth, keyHeight );

			if( !prevWhiteKeyState ) // key on, draw shadow
			{
				// shadow falling on left of white key.
				pnts[0].X = x + 1; // top left
				pnts[0].Y = 0;
				pnts[1].X = x;		// bottom left
				pnts[1].Y = keyHeight;
				pnts[2].X = x + keyWidth / 4;		// bottom right
				pnts[2].Y = keyHeight;
				pnts[3].X = x + keyWidth / 8;		// top right
				pnts[3].Y = 0;

				// draw it.
				graphics->FillPolygon(&brushShadow,pnts, 4);
			}
		}
		else
		{
			graphics->DrawLine(&highlightPen, x+1, top_shadow, x+1, keyHeight); // white highlight on left
		}

		prevWhiteKeyState = getKeyState(key);

		key += 2;

		int t = key % 12;
		if( t == 1 || t == 6 ) // skip 'gaps' in black keys
		{
			key--;
		}
	}

	// black key shadows
	int kn = 0;
	if( baseKey_ == 21 ) // Special case for piano.
	{
		kn = 5;
	}
	key = baseKey_ + 1;
	int blackKeyWidth = (2*keyWidth + 1) / 3;
	int blackKeyHeight = (2*keyHeight + 1)/3;
	int blackKeyTopIndent = (blackKeyWidth+2) / 4;
	int highlightY = blackKeyHeight / 2;
	for( int wx = 0 ; wx < usableWidth ; wx += keyWidth )
	{
		int t = kn++ % 7;
		if( t != 0 && t != 3 )
		{
			int x = wx - blackKeyWidth / 2;

			switch(t)
			{
			case 1:
			case 4:
				x -= (blackKeyWidth+2) / 6;
				break;
			case 6:
			case 2:
				x += (blackKeyWidth+2) / 6;
				break;
			};

			int top_indent;
			if( getKeyState(key) ) // key on, draw highlight different
				top_indent = blackKeyTopIndent;
			else
				top_indent = (blackKeyTopIndent * 2 ) / 3;

			int shadowLength = blackKeyWidth / 4;
			int bottomshadowLength = shadowLength;
			if( getKeyState(key) && !getKeyState(key - 1)) // key down, bottom shadow reduced, unless white key down too
			{
				bottomshadowLength = bottomshadowLength / 4;
			}

			if( getKeyState(key - 1) && !getKeyState(key)) // left white key down, bottom shadow increased
			{
				bottomshadowLength += shadowLength;
			}
			if( bottomshadowLength > wx - x ) // prevent shadow overlapping adjacent white key
			{
				bottomshadowLength = wx - x;
			}

			// shadow of black key falling on left white key.
			pnts[0].X = x;					// bottom left of key
			pnts[0].Y = blackKeyHeight - 1;
			pnts[1].X = x + bottomshadowLength;	// bottom left of shadow
			pnts[1].Y = blackKeyHeight + bottomshadowLength;
			pnts[2].X = wx;						// bottom right
			pnts[2].Y = blackKeyHeight + bottomshadowLength;
			pnts[3].X = wx;						// top right
			pnts[3].Y = blackKeyHeight - 1;

			graphics->FillPolygon(&brushShadow,pnts, 4 );


			// shadow of black key falling on right white key.
			bottomshadowLength = shadowLength;
			if( getKeyState(key) && !getKeyState(key + 1)) // key down, bottom shadow reduced, unless white key down too
			{
				bottomshadowLength = bottomshadowLength / 4;
			}
			if( getKeyState(key + 1) && !getKeyState(key)) // left white key down, bottom shadow increased
			{
				bottomshadowLength += shadowLength;
			}
			pnts[0].X = wx;					// bottom center of key
			pnts[0].Y = blackKeyHeight - 1;
			pnts[1].X = wx;	// bottom left of shadow
			pnts[1].Y = blackKeyHeight + bottomshadowLength;
			pnts[2].X = x + blackKeyWidth + bottomshadowLength;		// bottom right
			pnts[2].Y = blackKeyHeight + bottomshadowLength;
			pnts[3].X = x + (blackKeyWidth * 5) / 4 + (bottomshadowLength + shadowLength) / 2;		// middle right
			pnts[3].Y = blackKeyHeight / 2 + shadowLength;
			pnts[4].X = x + blackKeyWidth + shadowLength;		// top right
			pnts[4].Y = 0;
			pnts[5].X = x;		// top left
			pnts[5].Y = 0;

			// draw it.
			graphics->FillPolygon(&brushShadow,pnts, 6 );

			key += 2;

			int t2 = key % 12;
			if( t2 == 0 || t2 == 5 ) // skip 'gaps' in black keys
			{
				key++;
			}
		}
	}

	Pen pen( Color(transparency, 30, 30, 30), 1.f); // blackish

	// white keys
	key = baseKey_;
	for( int x = 0 ; x < usableWidth ; x += keyWidth )
	{
		graphics->DrawLine(&pen,x, 0, x, keyHeight); // black line between keys.

		key += 2;

		int t = key % 12;
		if( t == 1 || t == 6 ) // skip 'gaps' in black keys
		{
			key--;
		}
	}

	// black keys
	kn = 0;
	if( baseKey_ == 21 ) // Special case for piano.
	{
		kn = 5;
	}
	key = baseKey_ + 1;

	SolidBrush brush(Color::Black);
	grad1 = PointF(0.f,0.f);
	grad2 = PointF(0.f, (float)blackKeyHeight);
	LinearGradientBrush gradientBrush( grad1, grad2, Color::Black, Color::Gray );
	LinearGradientBrush gradientBrush3( grad1, PointF(0.f, (float)blackKeyHeight / 2.0f),  Color::Gray, Color::Black );

	// trapezoid on left of black keys
	for( int wx = 0 ; wx < usableWidth ; wx += keyWidth )
	{
		int t = kn++ % 7;
		if( t != 0 && t != 3 )
		{
			int x = wx - blackKeyWidth / 2;

			switch(t)
			{
			case 1:
			case 4:
				x -= (blackKeyWidth+2) / 6;
				break;
			case 6:
			case 2:
				x += (blackKeyWidth+2) / 6;
				break;
			};

			int top_indent;
			if( getKeyState(key) ) // key on, draw highlight different
				top_indent = blackKeyTopIndent;
			else
				top_indent = (blackKeyTopIndent * 2 ) / 3;

			// key background.
			graphics->FillRectangle( &brush, x, -1, blackKeyWidth, blackKeyHeight );
			// highlight on upper half.
			if( getKeyState(key) ) // key on, draw highlight different
			{
				graphics->FillRectangle( &gradientBrush3, x+blackKeyTopIndent, top_indent, blackKeyWidth-2*blackKeyTopIndent, blackKeyHeight/2 - top_indent);
			}
			else
			{
				graphics->FillRectangle( &gradientBrush, x+blackKeyTopIndent, top_indent, blackKeyWidth-2*blackKeyTopIndent, blackKeyHeight/2 - top_indent);
			}
//			graphics->FillRectangle( &gradientBrush, x+blackKeyTopIndent, blackKeyTopIndent, blackKeyWidth-2*blackKeyTopIndent, blackKeyHeight/2 - blackKeyTopIndent);
//			graphics->DrawLine(&highlightPen,x+blackKeyTopIndent, keyHeight/4,x+blackKeyTopIndent, keyHeight/2-blackKeyTopIndent*2);

			pnts[0].X = x + 1;
			pnts[0].Y = 0;
			pnts[1].X = x+blackKeyTopIndent;
				pnts[1].Y = top_indent;
			if( getKeyState(key) ) // key on, draw highlight different
			{
				pnts[2].X = x+blackKeyTopIndent / 2;
				pnts[2].Y = blackKeyHeight-(blackKeyTopIndent*4)/3;
			}
			else
			{
				pnts[2].X = x+blackKeyTopIndent;
				pnts[2].Y = blackKeyHeight-blackKeyTopIndent*2;
			}
			pnts[3].X = x + 1;
			pnts[3].Y = blackKeyHeight;

			// gradient
			PointF grad3((float)x, 0.f);
			PointF grad4((float) (x+blackKeyTopIndent), 0.f);
			LinearGradientBrush gradientBrush2( grad3, grad4, Color::Black, Color::Gray );

			// draw it.
			graphics->FillPolygon(&gradientBrush2,pnts, 4);

			key += 2;

			int t2 = key % 12;
			if( t2 == 0 || t2 == 5 ) // skip 'gaps' in black keys
			{
				key++;
			}
		}
	}

/* display debug text
   // Initialize arguments.
   Font myFont(L"Arial", 16);
   PointF origin(0.0f, 0.0f);
   SolidBrush blackBrush(Color(255, 255, 0, 0));
   StringFormat format;
   format.SetAlignment(StringAlignmentNear);

   // Draw string.
	graphics->DrawString(debugMessage, (int) wcslen(debugMessage), &myFont,
   origin,
   &format,
   &blackBrush);
*/

	// copy image buffer to actual screen.
	Graphics graphics2( hDC );
	graphics2.DrawImage( &bmp, 0, 0, width, keyHeight );

	delete graphics;
	return gmpi::MP_OK;
}

void KeyboardGui::onValueChanged( int index )
{
//	if(!m_inhibit_update)
	{
		invalidateRect();
	}
}

int32_t KeyboardGui::onLButtonDown( UINT flags, POINT point )
{
	setCapture();

	int key = PointToKeyNum(point);

	if( GetKeyState( VK_CONTROL ) < 0 ) // <ctrl> key toggles.
	{
		PlayNote( key, ! getKeyState(key) );
	}
	else
	{
		PlayNote( key, true );
	}

//	_RPT1(_CRT_WARN, "%d\n", key );

	return gmpi::MP_OK;
}

int32_t KeyboardGui::onMouseMove(UINT flags, POINT point)
{
	// testing
	int t = PointToKeyNum(point);

	if(!getCapture())
		return gmpi::MP_OK;

	int newMidiNote = PointToKeyNum(point);
	if( midiNote_ != newMidiNote )
	{
		int oldMidiNote = midiNote_;

		PlayNote( newMidiNote, true );

		// turn off current note
		if( !toggleMode_ && oldMidiNote > -1 )
		{
			PlayNote( oldMidiNote, false );
		}
	}

	return gmpi::MP_OK;
}

int32_t KeyboardGui::onLButtonUp(UINT flags, POINT point)
{
	if(!getCapture())
		return gmpi::MP_OK;

	if( GetKeyState( VK_CONTROL ) >= 0 && GetKeyState( VK_SHIFT ) >= 0 ) // <ctrl> key toggles. <shft> adds to selection.
	{
		PlayNote( midiNote_, false );
		midiNote_ = -1;
	}

	releaseCapture();

	return gmpi::MP_OK;
}

void KeyboardGui::PlayNote(int p_note_num, bool note_on)
{
	if( toggleMode_ )
	{
		if( !note_on )
			return;

		if( getKeyState(p_note_num) && note_on )
		{
			note_on = false;
		}
	}

	if( note_on )
	{
/*		// turn off current note
		if( !toggleMode_ && midiNote_ > -1 )
		{
			generator->Notify Ug( NUG_NOTE_OFF, midiNote_ );
			key State_[midiNote_] = false;
		}
*/
		const float middleA = 69.0f;
		const float notesPerOctave = 12.0f;
		midiNote_ = p_note_num;

		// Must send Pitch and Velocity before gate, else Patch-Automator MIDI output garbled.
		pinVelocities.setValue( p_note_num, 8.0f );
// no, overrides MTS		pinPitches.setValue( p_note_num, 5.0f + ( (float)( p_note_num - middleA ) / notesPerOctave ) );
		pinGates.setValue( p_note_num, 10.0f );
	}
	else
	{
		if( getKeyState(p_note_num) == false ) // seem to get ghost key off msg on sound start?, don't know why
			return;

		// NOTE: should setting this generate OnValueChanged() via host?, would be more consistant???
		pinGates.setValue(p_note_num, 0.0f);
		//pinTriggers.setValue(p_note_num, 0.0f);
	}
	
	invalidateRect();
}

int32_t KeyboardGui::measure(MpSize availableSize, MpSize &returnDesiredSize)
{
	const int prefferedSizeX = 200;
	const int prefferedSizeY = 40;
	const int minSize = 15;

	returnDesiredSize.x = availableSize.x;
	returnDesiredSize.y = availableSize.y;
	if(returnDesiredSize.x > prefferedSizeX)
	{
		returnDesiredSize.x = prefferedSizeX;
	}
	else
	{
		if(returnDesiredSize.x < minSize)
		{
			returnDesiredSize.x = minSize;
		}
	}
	if(returnDesiredSize.y > prefferedSizeY)
	{
		returnDesiredSize.y = prefferedSizeY;
	}
	else
	{
		if(returnDesiredSize.y < minSize)
		{
			returnDesiredSize.y = minSize;
		}
	}
	return gmpi::MP_OK;
}