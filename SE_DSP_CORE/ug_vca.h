#pragma once

#include "ug_base.h"

#define VCA_TABLE_SIZE_30V 1200
#define VCA_TABLE_SCALE_30V 400

class ug_vca : public ug_base
{
public:
	void OnCurveChange();
	static void InitVolTable(ug_base* p_ug, ULookup * &table);
	static void InitVolTable_old(ug_base* p_ug, ULookup * &table);
	ug_vca();
	int Open() override;
	float CalcGain( float vol );
	void process_both_run(int start_pos, int sampleframes);
	void process_both_run_linear(int start_pos, int sampleframes);
	void process_both_stop(int start_pos, int sampleframes);
	void process_vol_fixed(int start_pos, int sampleframes);
	void process_bypass(int start_pos, int sampleframes);
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state ) override;
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_vca);

	bool PPGetActiveFlag() override;
private:
	float fixed_gain;
	short scale_type;

	ULookup* shared_lookup_table;
	ULookup* shared_lookup_table_db;
	ULookup* shared_lookup_table_db2;
	float* m_lookup_table;	// pointer to shared one (easier for asm )

	float* in1_ptr;
	float* in2_ptr;
	float* out_ptr;

	float staticLevel_;
};
