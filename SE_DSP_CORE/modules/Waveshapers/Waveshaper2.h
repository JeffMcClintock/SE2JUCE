// Copyright 2006 Jeff McClintock

#ifndef WAVESHAPER2_H_INCLUDED
#define WAVESHAPER2_H_INCLUDED

#include "./waveshapers.h"

class Waveshaper2 : public Waveshaper
{
public:
	Waveshaper2(IMpUnknown* host) : Waveshaper(host) {}

protected:
	virtual void FillLookupTable();
};

#endif
