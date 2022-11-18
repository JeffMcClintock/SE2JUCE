// ug_midi_controllers module
//
#pragma once

#include "ug_midi_device.h"
#include "smart_output.h"
#include <vector>
#include "mp_midi.h"

// set number of custom controllers
#define UMIDICONTROL_NUM_CONTROLS 4

class ug_midi_controllers : public ug_midi_device
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_midi_controllers);
	ug_midi_controllers();
	int Open() override;

	void Resume() override;
	void ChangeOutput(timestamp_t p_sample_clock, UPlug* plug, float new_val);
	void sub_process(int start_pos, int sampleframes);
	void OnMidiData( int size, unsigned char* midi_bytes ) override;
	void onMidi2Message(const gmpi::midi::message_view& msg);

	short controller_type[UMIDICONTROL_NUM_CONTROLS + 1];
	unsigned short controller_val[UMIDICONTROL_NUM_CONTROLS + 1];

private:
	gmpi::midi_2_0::MidiConverter2 midiConverter;
	std::vector<smart_output> output_info;
};
