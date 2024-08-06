// ug_math_ceil module

//

#pragma once

#include "ug_generic_1_1.h"



class ug_math_ceil : public ug_generic_1_1

{

public:

	DECLARE_UG_BUILD_FUNC(ug_math_ceil);

	void sub_process(int start_pos, int sampleframes) override;

};

