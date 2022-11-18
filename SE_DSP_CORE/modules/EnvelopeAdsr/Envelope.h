#pragma once
#include "../se_sdk3/mp_sdk_audio.h"

#define FAST_DECAY_SAMPLES 15

class EnvelopeBase : public MpBase
{
public:
	EnvelopeBase( IMpUnknown* host );
	int32_t MP_STDCALL open() override;
	void OnGateChange( int blockPosition, float gate, float trigger );
	virtual void ChooseSubProcess( int blockPosition ) = 0;
	void sub_process( int start_pos, int sampleframes );
	void process_with_gate( int start_pos, int sampleframes );
	void sub_process3( int start_pos, int sampleframes );
	void sub_process4( int start_pos, int sampleframes );
	void sub_process5b( int start_pos, int sampleframes );
	void sub_process6( int start_pos, int sampleframes );
	void sub_process7( int start_pos, int sampleframes );
	void next_segment( int blockPosition );

	AudioInPin pinGate;
	AudioInPin pinTrigger;
	AudioInPin pinOverallLevel;
	AudioOutPin pinSignalOut;
	int sampleframesRemain;

protected:
	short sustain_segment;
	short end_segment;
	short cur_segment;
	short num_segments;
	// rearranged for speed in sub_process6(), didn't help
	int fixed_segment_time_remain;
	float fixed_increment;
	float output_val;
	float m_scaled_increment;
	float m_scaled_output_val;
	SubProcess_ptr current_segment_func;
	bool gate_state;
	bool trigger_state;
	float fixed_level;
	float fixed_segment_level;
	bool hold_level;
};


class Envelope : public EnvelopeBase
{
public:
	Envelope( IMpUnknown* host );
	inline float CalcIncrement( float p_rate );
	void onSetPins( void ) override;
	void ChooseSubProcess( int blockPosition );
	void sub_process5( int start_pos, int sampleframes );
	void sub_process8( int start_pos, int sampleframes );
	void sub_process9( int start_pos, int sampleframes );

	// 8 stage envelope maximum.
	enum { ENV_STEPS = 8 };
	float fixed_levels[ENV_STEPS];
	AudioInPin* rate_plugs[ENV_STEPS];
	AudioInPin* level_plugs[ENV_STEPS];

protected:
	FloatInPin pinVoiceReset;
};


