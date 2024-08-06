#pragma once

#include "ug_control.h"

#define FAST_DECAY_SAMPLES 15



class ug_envelope_base : public ug_control

{

public:

	void OnGateChange(float p_gate);

	virtual void ChooseSubProcess() = 0;



	void sub_process(int start_pos, int sampleframes);

	void process_with_gate(int start_pos, int sampleframes);



	void sub_process3(int start_pos, int sampleframes);

	void sub_process4(int start_pos, int sampleframes);

	void sub_process5b(int start_pos, int sampleframes);

	void sub_process6(int start_pos, int sampleframes);

	void sub_process7(int start_pos, int sampleframes);



	ug_envelope_base();

	int Open() override;

	void Resume() override;

	virtual void next_segment( void );

	void VoiceReset();



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



	// store various plug indexs, for faster access

	int pn_output;

	int pn_gate;

	int pn_level;



	process_func_ptr current_segment_func;

	bool gate_state;

	float fixed_level;

	float fixed_segment_level;



	bool hold_level;

	bool decay_mode;



	// pointers to plug datablocks

	float* output_ptr;

	float* level_ptr;

	float* gate_ptr;

};

