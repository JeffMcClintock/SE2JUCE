#pragma once

#include "ug_base.h"



class ug_io_mod : public ug_base

{

public:

	DECLARE_UG_INFO_FUNC2;

	DECLARE_UG_BUILD_FUNC(ug_io_mod);

	ug_io_mod();

	int Open() override;

};

