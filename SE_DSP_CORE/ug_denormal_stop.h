#pragma once

#include "ug_generic_1_1.h"



class ug_denormal_stop : public ug_generic_1_1

{

public:

	DECLARE_UG_BUILD_FUNC(ug_denormal_stop);

	void sub_process(int start_pos, int sampleframes) override;

};

