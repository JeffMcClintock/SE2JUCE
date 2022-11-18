// Copyright 2006 Jeff McClintock

#ifndef TriggerScope_dsp_H_INCLUDED
#define TriggerScope_dsp_H_INCLUDED

#include <limits.h>
#include "Scope3.h"

class TriggerScope : public Scope3
{
public:
	TriggerScope(IMpUnknown* host);
	virtual AudioInPin* getTriggerPin()
	{
		return &pinTrigger;
	}
	virtual int getTimeOut()
	{
		return INT_MAX;
	}

	// pins
	AudioInPin pinTrigger;
};

#endif
