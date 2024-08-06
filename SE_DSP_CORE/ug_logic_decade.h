#pragma once

#include "ug_logic_counter.h"



class ug_logic_decade : public ug_logic_counter

{

public:

	DECLARE_UG_BUILD_FUNC(ug_logic_decade);

	DECLARE_UG_INFO_FUNC2;

	int Open() override;

};

