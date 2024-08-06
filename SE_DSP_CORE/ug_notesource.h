// ug_notesource module
//
#pragma once

#include "ug_base.h"
#include "UMidiBuffer2.h"

class ug_notesource : public ug_base
{
public:
	struct s_chan_info // info that needs to be tracked per channel
	{
		float bender; // 0-1
		float midi_bend_range; // in semitones
	};

	ug_notesource();

	//	virtual bool ClonesVoices();
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state ) override;
	virtual void sub_process(int /*start_pos*/, int /*sampleframes*/) {}
//	virtual void PolyphonicSetup() {}
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_notesource);

	virtual void OnChangeChannel() {}
	virtual void OnChangeBank(int /*chan*/, int /*p_bank*/) {}
	int getVoiceAllocationMode();
	void updateVoiceAllocationMode();

protected:
	short mono_mode_old;
	short retrigger_old;
	short m_mono_note_priority;
	short m_poly_mode_old;
	short midi_channel;
	int cur_channel;
	short m_portamento_constant_rate;
	short m_auto_glide;
	//	s_chan_info* chan_info;  // stores current bender value 0-1, one per chan

	/*
	// Outputs
	float gate;
//	float velocity;

	//virtual void Retune(float pitch) {}
	//virtual void PitchBend(float bend_amt) {}

	//	short voice_channel;
	short voice_note;

protected:
	short pitch_bend_sensitity_raw;
	unsigned short incoming_rpn;

private:
	*/
};
