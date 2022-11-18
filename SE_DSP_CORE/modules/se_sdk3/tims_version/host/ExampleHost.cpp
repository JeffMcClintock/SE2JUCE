/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

/*
 * ExampleHost
 *
 * This is an implementation of a minimal GMPI host class in C.
 */

#include <stdio.h>
#include <stdlib.h>
#include "gmpi.h" /* JM-MOD */
#include "Host.h" /* JM-MOD */
#include "Guid.h" /* JM-MOD */
#include "lib.h" /* JM-MOD */
#include "ExampleHost.h"

/*
 * ExampleHost methods
 */

GMPI_Result
 ExampleHost::Placeholder1(void)
{
	GMPI_TRACE("%p ExampleHost::Placeholder1()\n", this);
	return GMPI_SUCCESS;
}

ExampleHost::ExampleHost(void)
{
	GMPI_TRACE("%p ExampleHost::ExampleHost()\n", this);
}

ExampleHost::~ExampleHost(void)
{
	GMPI_TRACE("%p ExamplePlugin::~ExamplePlugin()\n", this);
}
