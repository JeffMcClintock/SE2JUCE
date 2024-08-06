#pragma once
#include "ug_base.h"

class ug_control : public ug_base
{
public:
	ug_control() :
	controller_number(-1)
	,nrpn(0)
	,ignore_prog_change(0)
	{
		SET_CUR_FUNC( &ug_base::process_sleep );
	};

//	DECLARE_UG_INFO_FUNC2;

protected:
	short controller_number;
	short nrpn;
	bool ignore_prog_change;
};
