#include "mp_sdk_gui2.h"
#include "Drawing.h"
#include "DrawKeyboard.h"
#include "Keyboard2Gui.h"
#include "../se_sdk3/mp_midi.h"

using namespace gmpi;
using namespace GmpiMidi;
using namespace GmpiDrawing;

SE_DECLARE_INIT_STATIC_FILE(KeyboardMidiGui);

class KeyboardMidiGui : public KeyboardBase
{
public:
	KeyboardMidiGui()
	{
	}

	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override
	{
		return DrawKeyBoard::Render(drawingContext, getRect(), keyStates, baseKey_);
	}

	void PlayNote(int p_note_num, bool note_on) override
	{
		unsigned char midiMessage[]{0, static_cast<unsigned char>(p_note_num), 64};
		midiMessage[0] = note_on ? MIDI_NoteOn : MIDI_NoteOff;

		getHost()->sendMessageToAudio(0, sizeof(midiMessage), midiMessage);
	}
};

namespace
{
	auto r = Register<KeyboardMidiGui>::withId(L"SE Keyboard (MIDI)");
}
