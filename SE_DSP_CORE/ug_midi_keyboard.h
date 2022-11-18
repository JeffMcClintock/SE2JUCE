// ug_midi_keyboard module
//
#pragma once
#include <list>
#include "ug_base.h"
#include "UMidiBuffer2.h"

class ug_midi_keyboard : public ug_base
{
public:
	void sub_process(int start_pos, int sampleframes);
	ug_midi_keyboard();
	void OnNoteOff(int p_note_num);
	void OnNoteOn(int p_note_num);
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_midi_keyboard);
	void OnMidiData(unsigned int midi_data, timestamp_t timeStamp);

	// Compatibility with "SE Keyboard (MIDI)" which may be substituted in DX GUIs.
	void OnUiNotify2(int p_msg_id, my_input_stream& p_stream) override;

private:
	std::list<int> buffer; // internal midi buffer

	// outputs
	midi_output midi_out;
	short channel;
};
