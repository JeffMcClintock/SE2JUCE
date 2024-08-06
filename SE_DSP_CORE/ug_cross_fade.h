// ug_cross_fade module
// pragman onceif !defined(_ug_cross_fade_h_)
#define _ug_cross_fade_h_

#include "ug_pan.h"
#include "ULookup.h"

class ug_cross_fade : public ug_pan
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_cross_fade);

	ug_cross_fade();
	void onSetPin(timestamp_t p_clock, UPlug* p_to_plug, state_type p_state) override;
	void sub_process(int start_pos, int sampleframes);
	void sub_process_mix(int start_pos, int sampleframes);
	void sub_process_mix_linear(int start_pos, int sampleframes);
	void sub_process_static(int start_pos, int sampleframes);
private:
	float* input2_ptr;
};
