// ug_cv_midi module
//
#pragma once

#include "ug_base.h"
#include "UMidiBuffer2.h"

class ug_cv_midi : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_cv_midi);

	ug_cv_midi();
	void sub_process(int start_pos, int sampleframes);
	void gate_changed();

	int Open() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
private:
	float* gate_ptr;

	midi_output midi_out;
	bool gate_state;
	short channel;
	int midi_note;
	short freq_scale;
};
