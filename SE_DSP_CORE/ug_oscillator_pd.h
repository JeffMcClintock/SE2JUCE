#pragma once
#include <vector>
#include "sample.h"
#include "ug_oscillator2.h"

enum osc_waveform2;// {OWS2_SINE,OWS2_SAW,OWS2_RAMP,OWS2_PULSE,OWS2_TRI,OWS2_NOISE,OWS2_SILENCE};
#define SINE_TABLE_SIZE 512
#define SAW_TABLE_SIZE 512

class ug_oscillator_pd : public ug_base
{
public:
	struct pd_segment_info
	{
		float phase;
		float phase2;
		float mod_ammount;
	};
	//	inline float get_sample( float *wavedata, const unsigned int count) const;
	void FillSegmentTable();
	void ChooseProcessFunction();
	void sub_process(int start_pos, int sampleframes);
	void sub_process_fp(int start_pos, int sampleframes);
	void sub_process_overload(int start_pos, int sampleframes);
	void sub_process_overload_pitch(int start_pos, int sampleframes);
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	inline static unsigned int FrequencyToIntIncrement( float sampleRate, double freq_hz )
	{
		double temp_float = (UINT_MAX+1.f) * freq_hz / sampleRate + 0.5f;
		return (unsigned int) temp_float;
	};
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_oscillator_pd);
	ug_oscillator_pd();
	int Open() override;

	inline unsigned int calc_increment(float p_pitch);

	unsigned int count;
	unsigned int fixed_increment;

	// lookup table for wavetable increment
	ULookup_Integer* freq_lookup_table;

	short wave1;
	short wave2;

	wavetable_array sine_waves;

private:
	float m_dc_filter_const;
	float dc_offset;
	bool m_freq_overload;
	int m_wrap_idx[8];
	bool m_reso_section[8];
	int m_cur_cycle;
	pd_segment_info segments[18];
	int m_segment;

	short freq_scale;
	float* m_wave_data;
	float* pitch_ptr;
	float* modulation_ptr;
	float* output_ptr;
};
