/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_HOSTINTF_HPP_INCLUDED
#define GMPI_HOSTINTF_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "UnknownIntf.h" /* JM-MOD */

class GMPI_HostIntf: public GMPI_UnknownIntf
{
public:
	GMPI_HostIntf(IGMPI_Host* iplugin);
	GMPI_HostIntf(IGMPI_Unknown* iunknown);
	~GMPI_HostIntf(void);

	GMPI_Result Placeholder1(void);

protected:
	IGMPI_Host* m_interface;
};

/*
 * C++ to C thunks
 */

inline GMPI_Result
GMPI_HostIntf::Placeholder1()
{
	GMPI_TRACE("%p GMPI_HostIntf::Placeholder1()\n", this);
	return m_interface->methods->Placeholder1(m_interface);
}

inline
GMPI_HostIntf::GMPI_HostIntf(IGMPI_Host* ihost)
	: GMPI_UnknownIntf(reinterpret_cast<IGMPI_Unknown*>(ihost))
{
	GMPI_TRACE("%p GMPI_HostIntf::GMPI_HostIntf(%p)\n", this, ihost);
	m_interface = ihost;
}

inline
GMPI_HostIntf::GMPI_HostIntf(IGMPI_Unknown* iunknown)
	: GMPI_UnknownIntf(iunknown)
{
	GMPI_TRACE("%p GMPI_HostIntf::GMPI_HostIntf(%p)\n", this, iunknown);
	m_interface = reinterpret_cast<IGMPI_Host*>(iunknown);
}

inline
GMPI_HostIntf::~GMPI_HostIntf(void)
{
	GMPI_TRACE("%p GMPI_HostIntf::~GMPI_HostIntf()\n", this);
}

#endif /* GMPI_HOSTINTF_HPP_INCLUDED */
