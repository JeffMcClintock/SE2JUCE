#pragma once

#include "ug_generic_1_1.h"



class ug_denormal_detect : public ug_generic_1_1

{

public:

	DECLARE_UG_BUILD_FUNC(ug_denormal_detect);

	void sub_process(int start_pos, int sampleframes) override;

};

