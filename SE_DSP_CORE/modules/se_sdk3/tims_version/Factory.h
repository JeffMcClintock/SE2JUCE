/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_FACTORY_HPP_INCLUDED
#define GMPI_FACTORY_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "Unknown.h" /* JM-MOD */
#include "Guid.h" /* JM-MOD */
#include "HostIntf.h" /* JM-MOD */

class GMPI_Factory: public virtual GMPI_Unknown
{
public:
	virtual GMPI_Result CreateInstance(const GMPI_Guid& iid,
			GMPI_HostIntf& host, const char** args,
			IGMPI_Unknown** object) = 0;
	virtual GMPI_Result GetMetadata(int index,
			GMPI_Metadata** metadata) = 0;
};

#endif /* GMPI_FACTORY_HPP_INCLUDED */
