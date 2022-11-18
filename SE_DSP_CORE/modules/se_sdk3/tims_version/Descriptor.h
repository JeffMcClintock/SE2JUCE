/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_DESCRIPTOR_HPP_INCLUDED
#define GMPI_DESCRIPTOR_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "Plugin.h" /* JM-MOD */
#include "HostIntf.h" /* JM-MOD */

typedef GMPI_Result (*GMPI_PluginCreateFunc)(GMPI_HostIntf& host,
		const char** args, IGMPI_Plugin** plugin);

class GMPI_Descriptor
{
public:
	GMPI_Descriptor(GMPI_PluginCreateFunc func, GMPI_Metadata& meta);

	virtual GMPI_Result CreateInstance(GMPI_HostIntf& host,
			const char** args, IGMPI_Plugin** plugin);
	virtual GMPI_Result GetMetadata(GMPI_Metadata** metadata);

private:
	GMPI_PluginCreateFunc m_createFunc;
	GMPI_Metadata* m_metadata;
};

#endif /* GMPI_DESCRIPTOR_HPP_INCLUDED */
