// ug_math_floor module

//

#pragma once

#include "ug_generic_1_1.h"



class ug_math_floor : public ug_generic_1_1

{

public:

	DECLARE_UG_BUILD_FUNC(ug_math_floor);

	void sub_process(int start_pos, int sampleframes) override;

};

