/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_SDKPLUGIN_HPP_INCLUDED
#define GMPI_SDKPLUGIN_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "Plugin.h" /* JM-MOD */
#include "HostIntf.h" /* JM-MOD */
#include "Guid.h" /* JM-MOD */

class GMPI_SdkPlugin: public IGMPI_Plugin, public GMPI_Plugin
{
public:
	GMPI_SdkPlugin(GMPI_HostIntf& host, GMPI_Metadata& metadata);
	virtual ~GMPI_SdkPlugin(void);

	/* IGMPI_Unknown methods */
	virtual GMPI_Result QueryInterface(const GMPI_Guid& iid,
			IGMPI_Unknown** iobject);
	virtual int32_t AddRef(void);
	virtual int32_t Release(void);

	/* IGMPI_Plugin methods */
	virtual GMPI_Result Process(uint32_t count);
	virtual GMPI_Result GetPinMetadata(int32_t index, GMPI_PinMetadata* metadata);

protected:
	int32_t m_refcount;
	//FIXME: should this be a reference?
	GMPI_HostIntf* m_host;
	//FIXME: should this be a reference?
	GMPI_Metadata* m_metadata;
};

#endif /* GMPI_SDKPLUGIN_HPP_INCLUDED */
