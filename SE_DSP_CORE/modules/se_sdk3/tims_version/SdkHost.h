/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_SDKHOST_HPP_INCLUDED
#define GMPI_SDKHOST_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "Host.h" /* JM-MOD */
#include "Guid.h" /* JM-MOD */

class GMPI_SdkHost: public IGMPI_Host, public GMPI_Host
{
public:
	GMPI_SdkHost(void);
	virtual ~GMPI_SdkHost(void);

	/* IGMPI_Unknown methods */
	virtual GMPI_Result QueryInterface(const GMPI_Guid& iid,
			IGMPI_Unknown** iobject);
	virtual int32_t AddRef(void);
	virtual int32_t Release(void);

	/* IGMPI_Host methods */
	virtual GMPI_Result Placeholder1(void);

protected:
	int32_t m_refcount;
};

#endif /* GMPI_SDKHOST_HPP_INCLUDED */
