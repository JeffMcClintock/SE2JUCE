// ug_fixed_values module
//
#pragma once

#include "ug_base.h"

class ug_fixed_values : public ug_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_fixed_values);

	int Open() override;
	virtual ug_base* Clone(CUGLookupList& UGLookupList) override;
};

class ug_fixed_values_base : public ug_base
{
public:
	int Open() override
	{
		int r = ug_base::Open();
		SET_CUR_FUNC(&ug_base::process_sleep);

		for (auto p : plugs)
		{
			if (p->DataType == DT_FSAMPLE)
			{
				p->TransmitState(SampleClock(), ST_STATIC);
			}
			else
			{
				SendPinsCurrentValue(SampleClock(), p);
			}
		}

		return r;
	}

	// Create pin variables on the fly.
	void SetupDynamicPlugs() override
	{
		SetupDynamicPlugs2();
	}
	virtual ug_base* Clone(CUGLookupList& UGLookupList) override;
};

class ug_fixed_values_text : public ug_fixed_values_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_fixed_values_text);
};

class ug_fixed_values_float : public ug_fixed_values_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_fixed_values_float);
};

class ug_fixed_values_int : public ug_fixed_values_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_fixed_values_int);
};

class ug_fixed_values_bool : public ug_fixed_values_base
{
public:
	DECLARE_UG_INFO_FUNC2;
	DECLARE_UG_BUILD_FUNC(ug_fixed_values_bool);
};

