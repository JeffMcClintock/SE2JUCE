#pragma once
#include "ug_midi_device.h"

// replaced by ug_patch_automator
class ug_midi_automator : public ug_midi_device
{
	friend class ug_patch_automator_out;
public:
	ug_midi_automator();
	DECLARE_UG_BUILD_FUNC(ug_midi_automator);
	DECLARE_UG_INFO_FUNC2;

private:
	//	bool m_send_automation on_patch_change;
	midi_output midi_out;
	midi_output midi_out_hidden;
};

class ug_patch_automator;

class ug_patch_automator_out : public ug_base
{
public:
	DECLARE_UG_BUILD_FUNC(ug_patch_automator_out);
	DECLARE_UG_INFO_FUNC2;

	void SendAutomation2( float p_normalised, int voiceId, int p_unified_controller_id, const wchar_t* sysexFormat, bool caused_by_patch_change, int* lastMidiValue);
	ug_patch_automator_out();
	int Open() override;
	void sendProgramChange( int program );
	void setMidiChannel( int channel );

	ug_patch_automator* automator_in;

protected:
	midi_output midi_out;
	timestamp_t last_out_timestamp;

private:
	void SendController(timestamp_t p_clock, short controller_id, short val);
	//midi_input midi_in_hidden;
	midi_output midi_out_hidden; // not used
	short midi_channel;
	// MIDI Note-on requires knowlege of previous pitch/Velocity changes.
	float voicePitch[128];
	float voiceVelocityOn[128];
	float voiceVelocityOff[128];
	int lastRpnHeader_;
};
