#pragma once

#include "ug_base.h"

class ug_voice_splitter : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_voice_splitter);

	ug_voice_splitter();
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	void sub_process(int start_pos, int sampleframes);
	//	void sub_process2(int start_pos, int sampleframes);
	void sub_process_static(int start_pos, int sampleframes);
	int Open() override;
	void OnNewSetting( void);

private:
	UPlug* current_output_plug;
	float* in_ptr;
	float* out_ptr;
	int voice_;
	int current_output_plug_number;
	int numOutputs;
};
