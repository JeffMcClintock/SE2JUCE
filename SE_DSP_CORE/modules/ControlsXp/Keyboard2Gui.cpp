#include "./Keyboard2Gui.h"
#include <vector>
#include "Drawing.h"
#include "DrawKeyboard.h"

using namespace gmpi;
using namespace GmpiDrawing;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, Keyboard2Gui, L"SE Keyboard2" );

Keyboard2Gui::Keyboard2Gui()
{
//	initializePin(pinPitches);
	initializePin(2, pinGates, static_cast<MpGuiBaseMemberIndexedPtr2>(&Keyboard2Gui::onValueChanged));
	initializePin(pinVelocities);
}

bool Keyboard2Gui::getKeyState(int voice)
{
	return pinGates.getValue(voice) > 0.f;
}

int32_t KeyboardBase::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext)
{
	return DrawKeyBoard::Render(drawingContext, getRect(), keyStates, baseKey_);
}

void Keyboard2Gui::onValueChanged(int voice)
{
	//	if(!m_inhibit_update)
	{
		keyStates[voice] = pinGates.getValue(voice) > 0.f;
		invalidateRect();
	}
}

int32_t KeyboardBase::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	// Let host handle right-clicks.
	if ((flags & gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON) == 0)
	{
		return gmpi::MP_OK; // Indicate successful hit, so right-click menu can show.
	}

	setCapture();

	int key = DrawKeyBoard::PointToKeyNum(point, getRect(), baseKey_);

	if ((flags & gmpi_gui_api::GG_POINTER_KEY_CONTROL) == 1) // <ctrl> key toggles.
	{
		DoPlayNote(key, !keyStates[key]);
	}
    else
	{
		DoPlayNote(key, true);
	}
//	_RPT1(_CRT_WARN, "%d\n", key);

	return gmpi::MP_HANDLED;
}

int32_t KeyboardBase::onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	if (!getCapture())
		return gmpi::MP_OK;

	int newMidiNote = DrawKeyBoard::PointToKeyNum(point, getRect(), baseKey_);

	if (midiNote_ != newMidiNote)
	{
		int oldMidiNote = midiNote_;

		DoPlayNote(newMidiNote, true);

		// turn off current note
		if (/*!toggleMode_ && */oldMidiNote > -1)
		{
			DoPlayNote(oldMidiNote, false);
		}
	}

	return gmpi::MP_OK;
}

int32_t KeyboardBase::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	if ((flags & gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON) == 0)
	{
		return gmpi::MP_UNHANDLED;
	}

	if (!getCapture())
		return gmpi::MP_OK;
    
//    if (GetKeyState(VK_CONTROL) >= 0 && GetKeyState(VK_SHIFT) >= 0 && midiNote_ >= 0) // <ctrl> key toggles. <shft> adds to selection.
	if (   (flags & gmpi_gui_api::GG_POINTER_KEY_CONTROL) == 0
		&& (flags & gmpi_gui_api::GG_POINTER_KEY_SHIFT) == 0
		&& midiNote_ >= 0
		) // <ctrl> key toggles.
	{
		DoPlayNote(midiNote_, false);
		midiNote_ = -1;
	}

	releaseCapture();

	return gmpi::MP_OK;
}

void Keyboard2Gui::PlayNote(int p_note_num, bool note_on)
{
	if (note_on)
	{
		const float middleA = 69.0f;
		const float notesPerOctave = 12.0f;

		// Must send Pitch and Velocity before gate, else Patch-Automator MIDI output garbled.
		pinVelocities.setValue(p_note_num, 8.0f);
// no. overrides MTS.		pinPitches.setValue(p_note_num, 5.0f + ((float)(p_note_num - middleA) / notesPerOctave));
		pinGates.setValue(p_note_num, 10.0f);
	}
	else
	{
		if (getKeyState(p_note_num) == false) // seem to get ghost key off msg on sound start?, don't know why
			return;

		// NOTE: should setting this generate OnValueChanged() via host?, would be more consistant???
		pinGates.setValue(p_note_num, 0.0f);
	}
}



