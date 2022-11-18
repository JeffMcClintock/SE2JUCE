#pragma once

#include "ug_midi_device.h"
#include "UMidiBuffer2.h"
#include "../se_sdk3/mp_midi.h"

class ug_midi_filter : public ug_midi_device
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_midi_filter);
	ug_midi_filter();
	void OnMidiData( int size, unsigned char* midi_bytes ) override;
	void onMidi2Message(const gmpi::midi::message_view& msg);

private:
	midi_output midi_out;
	midi_output midi_out2;
	short channel_hi;
	short channel_lo;
	short velocity_hi;
	short velocity_lo;
	short note_num_hi;
	short note_num_lo;
	short pinProgramChange;
	gmpi::midi_2_0::MidiConverter2 midiConverter;

	uint8_t noteOnVelocities[16][256] = {};
};

