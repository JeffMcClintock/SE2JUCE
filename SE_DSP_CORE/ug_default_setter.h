#pragma once

#include "ug_base.h"

// Every container has one of these.
// It's sole purpose is to provide connections to unused module inputs,
// and set their default value.

class ug_default_setter : public ug_base
{
public:
	DECLARE_UG_BUILD_FUNC(ug_default_setter);
	ug_default_setter();
	ug_base* Clone( CUGLookupList& UGLookupList ) override;
	int Open() override;
};

