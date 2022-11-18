/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_HOST_HPP_INCLUDED
#define GMPI_HOST_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "Unknown.h" /* JM-MOD */

class GMPI_Host: public virtual GMPI_Unknown
{
public:
	virtual GMPI_Result Placeholder1(void) = 0;
};

#endif /* GMPI_HOST_HPP_INCLUDED */
