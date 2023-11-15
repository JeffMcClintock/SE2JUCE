#pragma once
#include "ug_base.h"
#include "modules/shared/wav_file.h"

class ug_wave_player :
	public ug_base
{
public:
	ug_wave_player();
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_wave_player);

	int Open() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

	int open_file();
	void retrigger_sample(timestamp_t p_sampleclock);
	int calc_increment(float pitch_shift); // Re calc increment for a new freq

	void sub_process_file(int start_pos, int sampleframes);
	void sub_process_file_stereo(int start_pos, int sampleframes);
	void sub_process_file_mono(int start_pos, int sampleframes);
	void sub_process_file_silence(int start_pos, int sampleframes);

	static bool m_user_inhibit_error_message; // *STATIC* BUT HOPEFULLY HARMLESS.
private:
	bool refill_buffer(bool gate)
	{
		auto r = cursor->GetMoreSamples(gate);
		out_buffer = std::get<0>(r);
		s_ptr = out_buffer;
		s_end = out_buffer + std::get<1>(r);

		return std::get<1>(r) <= 0; // done when no samples remain.
	}

	// Inputs
	std::wstring FileName;
	short mode;
	float* gate_ptr;
	float* pitch_ptr;
	float* output[3];

	bool play_flag;

	int increment;
	int s_ptr_fine;
	const float* s_ptr;
	const float* s_end;
	const float* interpolation_table2;
	const float* out_buffer = nullptr; // [OUT_BUFFER_SIZE];
	process_func_ptr current_process_func;

	int n_channels = 2;
	float* buffer_pointer[3];
	bool m_inhibit_error_msg; // inhibit unnes repeated messages


	state_type plug_status_gate;
	state_type plug_status_pitch;
	int loop_start;
	int loop_end; // byte position
	bool gate_state_hi = false;
	bool loop_forever;
	std::wstring m_currently_loaded_file;
	WavFileStreaming wavReader;
	std::unique_ptr<WavFileCursor> cursor;

};
