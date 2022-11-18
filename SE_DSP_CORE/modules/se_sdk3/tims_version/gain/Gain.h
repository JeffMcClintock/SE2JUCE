/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_EXAMPLE_PLUGIN_HPP_INCLUDED
#define GMPI_EXAMPLE_PLUGIN_HPP_INCLUDED

#include "gmpi.h"
#include "SdkPlugin.h"
#include "HostIntf.h"

class Gain: public GMPI_SdkPlugin
{
public:
	Gain(GMPI_HostIntf& host);
	virtual ~Gain(void);

	/* create an instance */
	static GMPI_Result CreateInstance(GMPI_HostIntf& host,
			const char** args, IGMPI_Plugin** plugin);

	/* GMPI_Plugin methods */
	virtual GMPI_Result QueryInterface(const GMPI_Guid& iid, IGMPI_Unknown** interface);
	virtual GMPI_Result Process(int32_t count);
	virtual GMPI_Result GetPinMetadata(int32_t index, GMPI_PinMetadata* metadata);
};

#endif /* GMPI_EXAMPLE_PLUGIN_H_INCLUDED */
