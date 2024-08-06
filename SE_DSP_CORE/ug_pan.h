// ug_pan module
//
#pragma once

#include "ug_base.h"
#include "ug_vca.h"
#include "ULookup.h"

class ug_pan : public ug_base
{
public:
	void SetLookupTable();
	static void InitFadeTables(ug_base* p_ug, ULookup * &sin_table, ULookup * &sqr_table);
	void CalcFixedMultipliers(float p_pan);
	bool PPGetActiveFlag() override;
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_pan);

	ug_pan();
	int Open() override;
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	void sub_process(int start_pos, int sampleframes);
	void sub_process_pan(int start_pos, int sampleframes);
	void sub_process_pan_linear(int start_pos, int sampleframes);
	void sub_process_static(int start_pos, int sampleframes);

protected:
	inline float CalcGain(float p_volume) const
	{
		return fastInterpolationClipped2( p_volume * (float) VCA_TABLE_SCALE_30V, VCA_TABLE_SIZE_30V, shared_lookup_table_db->GetBlock() );
	}

	short fade_type;
	ULookup* shared_lookup_table;
	ULookup* shared_lookup_table_sin;
	ULookup* shared_lookup_table_sqr;
	ULookup* shared_lookup_table_db;
	float multiplier_l;
	float multiplier_r;


	float* input1_ptr;
	float* pan_ptr;
	float* left_output_ptr;
	float* right_output_ptr;
	float* volume_ptr;
};
