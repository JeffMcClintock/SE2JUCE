/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_UNKNOWNINTF_HPP_INCLUDED
#define GMPI_UNKNOWNINTF_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "Guid.h" /* JM-MOD */

class GMPI_UnknownIntf
{
public:
	GMPI_UnknownIntf(IGMPI_Unknown* iunknown);
	~GMPI_UnknownIntf(void);

	/* IGMPI_Unknown methods */
	GMPI_Result QueryInterface(const GMPI_Guid& iid,
			IGMPI_Unknown** iobject);
	int32_t AddRef(void);
	int32_t Release(void);

protected:
	IGMPI_Unknown* m_interface;
};

/*
 * C++ to C thunks
 */

inline GMPI_Result
GMPI_UnknownIntf::QueryInterface(const GMPI_Guid& iid,
		IGMPI_Unknown** iobject)
{
	GMPI_TRACE("%p GMPI_UnknownIntf::QueryInterface()\n", this);
	return m_interface->methods->QueryInterface(m_interface,
			&iid, iobject);
}

inline int32_t
GMPI_UnknownIntf::AddRef(void)
{
	GMPI_TRACE("%p GMPI_UnknownIntf::AddRef()\n", this);
	return m_interface->methods->AddRef(m_interface);
}

inline int32_t
GMPI_UnknownIntf::Release(void)
{
	GMPI_TRACE("%p GMPI_UnknownIntf::Release()\n", this);
	return m_interface->methods->Release(m_interface);
}

inline
GMPI_UnknownIntf::GMPI_UnknownIntf(IGMPI_Unknown* iunknown)
{
	GMPI_TRACE("%p GMPI_UnknownIntf::GMPI_UnknownIntf(%p)\n", this,
			iunknown);
	/* the interface has a reference for us already */
	m_interface = iunknown;
}

inline
GMPI_UnknownIntf::~GMPI_UnknownIntf(void)
{
	GMPI_TRACE("%p GMPI_UnknownIntf::~GMPI_UnknownIntf()\n", this);
	/* release our reference on the underlying interface */
	Release();
}

#endif /* GMPI_UNKNOWNINTF_HPP_INCLUDED */
