/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_UNKNOWN_HPP_INCLUDED
#define GMPI_UNKNOWN_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "Guid.h" /* JM-MOD */

class GMPI_Unknown
{
public:
	virtual GMPI_Result QueryInterface(const GMPI_Guid& iid,
			IGMPI_Unknown** iobject) = 0;
	virtual int32_t AddRef(void) = 0;
	virtual int32_t Release(void) = 0;
};

#endif /* GMPI_UNKNOWN_HPP_INCLUDED */
