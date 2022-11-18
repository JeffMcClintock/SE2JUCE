/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

/*
 * GMPI_SdkHost
 *
 * This file provides a base class which implements the IGMPI_Host
 * interface.
 */

#include <stdlib.h>
#include "gmpi.h" /* JM-MOD */
#include "SdkHost.h" /* JM-MOD */
#include "Guid.h" /* JM-MOD */
#include "lib.h" /* JM-MOD */

/*
 * C to C++ thunks
 */

static GMPI_Result GMPI_STDCALL
GH_QueryInterface(IGMPI_Host* igh, const GMPI_Guid* iid,
		IGMPI_Unknown** iobject)
{
	GMPI_SdkHost* host = static_cast<GMPI_SdkHost*>(igh);
	return host->QueryInterface(*iid, iobject);
}

static int32_t GMPI_STDCALL
GH_AddRef(IGMPI_Host* igh)
{
	GMPI_SdkHost* host = static_cast<GMPI_SdkHost*>(igh);
	return host->AddRef();
}

static int32_t GMPI_STDCALL
GH_Release(IGMPI_Host* igh)
{
	GMPI_SdkHost* host = static_cast<GMPI_SdkHost*>(igh);
	return host->Release();
}

static GMPI_Result GMPI_STDCALL
GH_Placeholder1(IGMPI_Host* igh)
{
	GMPI_SdkHost* host = static_cast<GMPI_SdkHost*>(igh);
	return host->Placeholder1();
}

/* This defines the host methods interface for the C linkage */
static IGMPI_HostMethods GH_Methods = {
	GH_QueryInterface,
	GH_AddRef,
	GH_Release,
	GH_Placeholder1,
};


/*
 * Host methods
 */

GMPI_Result
GMPI_SdkHost::QueryInterface(const GMPI_Guid& iid, IGMPI_Unknown** iobject)
{
	GMPI_TRACE("%p GMPI_SdkHost::QueryInterface()\n", this);

	/*
	 * For SDK hosts, the root object (IGMPI_Unknown) is the same as
	 * the host object (IGMPI_Host).  It's simpler that way.
	 */
	if (iid == GMPI_IID_UNKNOWN || iid == GMPI_IID_HOST) {
		IGMPI_Host* ihost = static_cast<IGMPI_Host*>(this);
		*iobject = reinterpret_cast<IGMPI_Unknown*>(ihost);
		AddRef();
		return GMPI_SUCCESS;
	}

	*iobject = NULL;
	return GMPI_NOSUPPORT;
}

int32_t
GMPI_SdkHost::AddRef(void)
{
	m_refcount++;
	GMPI_TRACE("%p GMPI_SdkHost::AddRef() = %d\n", this, m_refcount);
	return m_refcount;
}

int32_t
GMPI_SdkHost::Release(void)
{
	m_refcount--;
	GMPI_TRACE("%p GMPI_SdkHost::Release() = %d\n", this, m_refcount);

	/*
	 * For SDK hosts, we assume they are dynamically created.  If
	 * you really need a statically allocated instance, call AddRef()
	 * once, so it never gets deleted.
	 */
	if (m_refcount == 0) {
		delete this;
		return 0;
	}
	return m_refcount;
}

GMPI_Result
GMPI_SdkHost::Placeholder1(void)
{
	GMPI_TRACE("%p GMPI_SdkHost::Placeholder1()\n", this);
	return GMPI_FAIL;
}

GMPI_SdkHost::GMPI_SdkHost(void)
{
	GMPI_TRACE("%p GMPI_SdkHost::GMPI_SdkHost()\n", this);
	IGMPI_Host::methods = &GH_Methods;
	m_refcount = 0;
}

GMPI_SdkHost::~GMPI_SdkHost(void)
{
	GMPI_TRACE("%p GMPI_SdkHost::~GMPI_SdkHost()\n", this);
}
