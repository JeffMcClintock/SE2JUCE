#pragma once

#include "ug_notesource.h"
#include "smart_output.h"
#include "ug_midi_device.h"
#include "IDspPatchManager.h"

#define MCV_NOTE_MEM_SIZE 16

class ug_midi_to_cv_redirect : public ug_midi_device
{
	VoiceControlState voiceState_;

public:
	ug_midi_to_cv_redirect() {}
	int Open() override;
	void OnMidiData(int size, unsigned char* midi_bytes) override;
};

class ug_midi_to_cv : public ug_notesource
{
public:
	float bender_to_voltage(float p_bend_amt);
	virtual void Resume() override;
	ug_midi_to_cv();
	int Open() override;
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_midi_to_cv);

	void sub_process(int start_pos, int sampleframes) override;
	void PitchBend(float bend_amt);
	virtual void Retune(float pitch);
	void Aftertouch(float val);
	void NoteOff(/*short chan,*/ short NoteNum, short NoteVel = 0);
	void NoteOn2( /*short chan,*/ short VoiceId, float velocity, float pitch);
	void delay_gate_on();
	void delay_gate_off();
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state ) override;
	virtual void BuildHelperModule() override;

private:
	short BendRange;
	int last_note;
	smart_output gate_so;
	smart_output velocity_so;
	smart_output aftertouch_so;

	// time saving
	UPlug* pitch_pl;

	float portamento_target;
	float portamento_increment;
	float portamento_pitch;

	float pitch_bend;
	float pitch_bend_target;
	float pitch_bend_increment;

	timestamp_t lastGateHi;
	bool voiceResetBetweenNotes;
	float m_voice_gate;
	float m_voice_active;
	float m_voice_trigger;
	float m_voice_velocity;
	float m_voice_pitch;
	float m_voice_aftertouch;
	float m_bend_amt;
	int m_voice_id;
	float m_bender;
	float m_portamento_enable;
	float m_hold_pedal;
//	float GlideStartPitch_;
	bool noteKeyDown_; // Is key on/off. Used for glide retrigger.
	bool ignoreNoteOnPitch_;
	bool m_held;
};
