/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_EXAMPLEHOST_HPP_INCLUDED
#define GMPI_EXAMPLEHOST_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "SdkHost.h" /* JM-MOD */

class ExampleHost: public GMPI_SdkHost
{
public:
	ExampleHost(void);
	virtual ~ExampleHost(void);

	/* GMPI_Host methods */
	virtual GMPI_Result Placeholder1(void);
};

#endif /* GMPI_EXAMPLEHOST_H_INCLUDED */
