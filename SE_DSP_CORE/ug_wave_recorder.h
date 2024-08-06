#pragma once
#include "ug_base.h"


#define MAX_SPEAKER_OUTPUTS 12
// patch to/source pc hard disk
#define BUF_SIZE 2048

class ug_wave_recorder : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_wave_recorder);

	ug_wave_recorder();
	~ug_wave_recorder();

	int Open() override;

	void sub_process_to_file_mono(int start_pos, int sampleframes);
	void sub_process_to_file_stereo(int start_pos, int sampleframes);
	void sub_process_to_file_float_mono(int start_pos, int sampleframes);
	void sub_process_to_file_float_stereo(int start_pos, int sampleframes);

	int Close() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;

	void TimeUp();
	void CloseFile();

private:
	float* input[MAX_SPEAKER_OUTPUTS];
	short m_format;
	int bits_per_sample;
	bool play_file;
	double total_time;
	bool debug;
	char buffer[BUF_SIZE];

	std::wstring FileName;
	FILE* fileHandle = {};
	int64_t start_time;
	void* wo_pointer = {};

	int open_file();
	void flush_buffer();

	timestamp_t sample_count = {};

	// various info from wave files
	SHORT wFormatTag, nBlockAlign, wBitsPerSample;
	int nSamplesPerSec, nAvgBytesPerSec, num_samples;
	int n_channels;
	short current_format;
};
