// ug_midi_device module
//
#pragma once

#include "ug_base.h"
#include "UMidiBuffer2.h"

class ug_midi_device : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_midi_device);

	ug_midi_device();
	int Open() override;
	void HandleEvent(SynthEditEvent* e) override;
	virtual void OnMidiData( int size, unsigned char* midi_bytes );

protected:
	//	midi_input midi_in;
	midi_output midi_out;
	short midi_channel;
};
