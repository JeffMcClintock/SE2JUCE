// ug_volts_to_float module
//
#pragma once

#include "ug_base.h"
#include "sample.h"

class ug_volts_to_float : public ug_base
{
	friend class Ctl_LED;
public:
	void assignPlugVariable(int p_plug_desc_id, UPlug* p_plug) override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	void sub_process_rms(int start_pos, int sampleframes);
	void sub_process_peak(int start_pos, int sampleframes);
	void sub_process_clip_detect(int start_pos, int sampleframes);
	void sub_process_dc(int start_pos, int sampleframes);
	void mode_change();
	void monitor();
	DECLARE_UG_BUILD_FUNC(ug_volts_to_float);

	ug_volts_to_float();
	int Open() override;

	float current_max;

protected:
	short mode;
	short response;
	float* input_ptr;
	float output_val;

private:
	float m_threshold;
	bool monitor_done;
	bool monitor_routine_running;
	float max_input;
	process_func_ptr audio_routine;
	ug_func monitor_routine;
	int delay;
	float* threshold_ptr;

	float m_attack;
	float m_release;
	float m_env;
	short m_update_hz;
	float y1n;
	float low_pass_cof;
};

class ug_volts_to_float2 : public ug_volts_to_float
{
public:
	ug_volts_to_float2()
	{
		SetFlag( UGF_POLYPHONIC_AGREGATOR, false ); // was causing problems in polyphonic signal paths, like wavetable osc. Proper place to agregate voices is patchmem anyhow.
	}
	DECLARE_UG_BUILD_FUNC(ug_volts_to_float2);
};
