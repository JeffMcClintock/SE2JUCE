/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

/*
 * GMPI_Descriptor
 *
 * This file provides a base class which implements a simple
 * GMPI_Descriptor.
 */

#include <stdlib.h>
#include "gmpi.h" /* JM-MOD */
#include "Descriptor.h" /* JM-MOD */
#include "Plugin.h" /* JM-MOD */
#include "HostIntf.h" /* JM-MOD */

GMPI_Descriptor::GMPI_Descriptor(GMPI_PluginCreateFunc createFunc,
		GMPI_Metadata& metadata)
{
	m_createFunc = createFunc;
	m_metadata = &metadata;
}

GMPI_Result
GMPI_Descriptor::CreateInstance(GMPI_HostIntf& host, const char** args,
		IGMPI_Plugin** plugin)
{
	return m_createFunc(host, args, plugin);
}

GMPI_Result
GMPI_Descriptor::GetMetadata(GMPI_Metadata** metadata)
{
	*metadata = m_metadata;
	return GMPI_SUCCESS;
}
