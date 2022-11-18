/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_EXAMPLE_PLUGIN_HPP_INCLUDED
#define GMPI_EXAMPLE_PLUGIN_HPP_INCLUDED

#include "gmpi/gmpi.h"
#include "gmpi/SdkPlugin.h"
#include "gmpi/HostIntf.h"

class NoopPlugin: public GMPI_SdkPlugin
{
public:
	NoopPlugin(GMPI_HostIntf& host);
	virtual ~NoopPlugin(void);

	/* create an instance */
	static GMPI_Result CreateInstance(GMPI_HostIntf& host,
			const char** args, IGMPI_Plugin** plugin);

	/* GMPI_Plugin methods */
	virtual GMPI_Result GMPI_STDCALL QueryInterface(const GMPI_Guid& iid,
			IGMPI_Unknown** iobject);
	virtual GMPI_Result GMPI_STDCALL Process(uint32_t count);
	virtual GMPI_Result GMPI_STDCALL Placeholder2(int32_t* arg);
};

#endif /* GMPI_EXAMPLE_PLUGIN_H_INCLUDED */
