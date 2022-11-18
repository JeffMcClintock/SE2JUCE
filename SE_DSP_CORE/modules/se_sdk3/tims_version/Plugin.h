/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_PLUGIN_HPP_INCLUDED
#define GMPI_PLUGIN_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "Unknown.h" /* JM-MOD */

class GMPI_Plugin: public virtual GMPI_Unknown
{
public:
	virtual GMPI_Result Process(uint32_t count) = 0;
	virtual GMPI_Result GetPinMetadata(int32_t index, GMPI_PinMetadata* metadata) = 0;
};

#endif /* GMPI_PLUGIN_HPP_INCLUDED */
