/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_FACTORYINTF_HPP_INCLUDED
#define GMPI_FACTORYINTF_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "UnknownIntf.h" /* JM-MOD */
#include "Host.h" /* JM-MOD */
#include "Guid.h" /* JM-MOD */

class GMPI_FactoryIntf: public GMPI_UnknownIntf
{
public:
	GMPI_FactoryIntf(IGMPI_Factory* ifactory);
	GMPI_FactoryIntf(IGMPI_Unknown* iunknown);
	~GMPI_FactoryIntf(void);

	GMPI_Result CreateInstance(const GMPI_Guid& iid,
			IGMPI_Host& host, const char** args,
			IGMPI_Unknown** object);

	GMPI_Result GetMetadata(int index, GMPI_Metadata** metadata);

protected:
	IGMPI_Factory* m_interface;
};

/*
 * C++ to C thunks
 */

inline GMPI_Result
GMPI_FactoryIntf::CreateInstance(const GMPI_Guid& iid,
		IGMPI_Host& host, const char** args,
		IGMPI_Unknown** object)
{
	IGMPI_Unknown* unhost = reinterpret_cast<IGMPI_Unknown*>(&host);
	GMPI_TRACE("%p GMPI_FactoryIntf::CreateInstance()\n", this);
	return m_interface->methods->CreateInstance(m_interface, &iid,
			unhost, args, object);
}

inline GMPI_Result
GMPI_FactoryIntf::GetMetadata(int index, GMPI_Metadata** metadata)
{
	GMPI_TRACE("%p GMPI_FactoryIntf::GetMetadata()\n", this);
	return m_interface->methods->GetMetadata(m_interface, index, metadata);
}

inline
GMPI_FactoryIntf::GMPI_FactoryIntf(IGMPI_Factory* ifactory)
	: GMPI_UnknownIntf(reinterpret_cast<IGMPI_Unknown*>(ifactory))
{
	GMPI_TRACE("%p GMPI_FactoryIntf::GMPI_FactoryIntf(%p)\n", this,
			ifactory);
	m_interface = ifactory;
}

inline
GMPI_FactoryIntf::GMPI_FactoryIntf(IGMPI_Unknown* iunknown)
	: GMPI_UnknownIntf(iunknown)
{
	GMPI_TRACE("%p GMPI_FactoryIntf::GMPI_FactoryIntf(%p)\n", this,
			iunknown);
	m_interface = reinterpret_cast<IGMPI_Factory*>(iunknown);
}

inline
GMPI_FactoryIntf::~GMPI_FactoryIntf(void)
{
	GMPI_TRACE("%p GMPI_FactoryIntf::~GMPI_FactoryIntf()\n", this);
}

#endif /* GMPI_FACTORYINTF_HPP_INCLUDED */
