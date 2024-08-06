#pragma once
#include <vector>
#include "sample.h"
#include "ug_base.h"
#include "ULookup.h"
#include "cancellation.h"

typedef std::vector<SharedOscWaveTable> wavetable_array;

#define SINE_TABLE_SIZE 512
#define SAW_TABLE_SIZE 512

class ULookup_Integer;

enum osc_waveform2 {OWS2_SINE,OWS2_COSINE,OWS2_SAW,OWS2_RAMP,OWS2_PULSE,OWS2_TRI,OWS2_NOISE,OWS2_SILENCE};

class ug_oscillator2 : public ug_base
{
public:
	inline float pulse_get_sample( float* wavedata, const unsigned int count, const unsigned int pulse_width) const;
	inline float pulse_get_sample2( float* wavedata, const unsigned int count, float pulseWidth) const;
	inline float get_sample( float* wavedata, const unsigned int count) const;
	inline unsigned int calc_phase( float phase, float modulation_depth );
	// compiler doesn't inline this without __forceinline, however, inline made no speed improvement, need to optimise code
	inline unsigned int calc_increment(float p_pitch);
	void switch_wavetable(unsigned int increment, unsigned int blockPos );
	static void InitPitchTable( ug_base* p_ug, ULookup_Integer * &table);
	static void InitPitchTableFloat( ug_base* p_ug, ULookup* &table);
	static void InitVoltToHzTable( ug_base* p_ug, ULookup* &table);
	static void FillWaveTable( ug_base* ug, float sampleRate, wavetable_array& wa, osc_waveform2 waveshape, bool p_gibbs_fix = true );
	void ChooseProcessFunction( int blockPos );
	void process_sync_crossfade(int start_pos, int sampleframes);
	void process_wave_crossfade(int start_pos, int sampleframes);
	void process_with_sync(int start_pos, int sampleframes);
	void process_pink_noise(int start_pos, int sampleframes);
	void process_white_noise(int start_pos, int sampleframes);

	void sub_process(int start_pos, int sampleframes);
	void sub_process_pitch(int start_pos, int sampleframes);
	void sub_process_phase(int start_pos, int sampleframes);
	void sub_process_fp(int start_pos, int sampleframes);

	void sub_process_ramp(int start_pos, int sampleframes);
	void sub_process_pitch_ramp(int start_pos, int sampleframes);
	void sub_process_phase_ramp(int start_pos, int sampleframes);
	void sub_process_fp_ramp(int start_pos, int sampleframes);


	void sub_process_pulse(int start_pos, int sampleframes);
	void sub_process_pitch_pulse(int start_pos, int sampleframes);
	void sub_process_phase_pulse(int start_pos, int sampleframes);
	void sub_process_fp_pulse(int start_pos, int sampleframes);

	void sub_process_pw_pulse(int start_pos, int sampleframes);
	void sub_process_pw_pitch_pulse(int start_pos, int sampleframes);
	void sub_process_pw_phase_pulse(int start_pos, int sampleframes);
	void sub_process_pw_fp_pulse(int start_pos, int sampleframes);

	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	inline static unsigned int FrequencyToIntIncrement( float sampleRate, double freq_hz )
	{
		double temp_float = (UINT_MAX+1.f) * freq_hz / sampleRate + 0.5f;
		return (unsigned int) temp_float;
	};

	//	void ChangePhase();
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_oscillator2);
	ug_oscillator2();
	int Open() override;
	void OnFirstSample();

	void ChangeWaveform( unsigned int blockPos );

	unsigned int count;
	unsigned int fixed_increment;
	unsigned int increment_hi_band;
	unsigned int increment_lo_band;
	OscWaveTable* cur_wave;

	// lookup table for wavetable increment
	ULookup_Integer* freq_lookup_table;
	ULookup* lookup_table_volt_to_hz;

	short waveform;
	unsigned int random;		// Used by Noise.

	OscWaveTable* next_wave;	// holds next wavetable (waiting for zero crossing)
	int sync_cf_offset;

	SharedOscWaveTable wave_overload;	// silence (used if freq > nyqest )
	wavetable_array sine_waves;  // not multithread safe!
	wavetable_array saw_waves;
	wavetable_array saw_waves_no_gibbs;
	wavetable_array tri_waves_no_gibbs;
	// pointer to current wavetable (1 per waveshape)
	wavetable_array* wavetable;

	// pink noise stuff
	float buf0;
	float buf1;
	float buf2;
	float buf3;
	float buf4;
	float buf5;

private:
	void DoSync( float p_phase, float p_phase_mod_depth, unsigned int p_zero_cross_offset );
	float m_prev_sync_sample;
	bool m_gibbs_fix;
	bool m_smooth_sync;
//	float debug_sin;
	unsigned int fixed_pulse_width;
	int sync_count_offset;
	int cross_fade_samples;
	int wave_fade_samples;
	process_func_ptr current_osc_func;
	process_func_ptr current_osc_func2;

	bool sync_flipflop;

	unsigned int cur_phase;
//	int sync_crossfade_cnt;
	short freq_scale;

	float* m_wave_data;

	float* pitch_ptr;
	float* pulse_width_ptr;
	float* sync_ptr;
	float* phase_mod_ptr;
	float* phase_mod_depth_ptr;
	float* output_ptr;
	float hz_to_inc_const;
};
