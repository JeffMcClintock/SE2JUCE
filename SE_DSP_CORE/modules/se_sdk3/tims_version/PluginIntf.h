/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_PLUGININTF_HPP_INCLUDED
#define GMPI_PLUGININTF_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "UnknownIntf.h" /* JM-MOD */

class GMPI_PluginIntf: public GMPI_UnknownIntf
{
public:
	GMPI_PluginIntf(IGMPI_Plugin* iplugin);
	GMPI_PluginIntf(IGMPI_Unknown* iunknown);
	~GMPI_PluginIntf(void);

	GMPI_Result Process(uint32_t count);
	GMPI_Result GetPinMetadata(int32_t arg, struct GMPI_PinMetadata *metadata);

protected:
	IGMPI_Plugin* m_interface;
};

/*
 * C++ to C thunks
 */

inline GMPI_Result
GMPI_PluginIntf::Process(uint32_t count)
{
	GMPI_TRACE("%p GMPI_PluginIntf::Process(%d)\n", this, count);
	return m_interface->methods->Process(m_interface, count);
}

inline GMPI_Result
GMPI_PluginIntf::GetPinMetadata(int32_t arg, struct GMPI_PinMetadata *metadata)
{
	GMPI_TRACE("%p GMPI_PluginIntf::Placeholder2(%p)\n", this, arg);
	return m_interface->methods->GetPinMetadata(m_interface, arg, metadata);
}

inline
GMPI_PluginIntf::GMPI_PluginIntf(IGMPI_Plugin* iplugin)
	: GMPI_UnknownIntf(reinterpret_cast<IGMPI_Unknown*>(iplugin))
{
	GMPI_TRACE("%p GMPI_PluginIntf::GMPI_PluginIntf(%p)\n", this,
			iplugin);
	m_interface = iplugin;
}

inline
GMPI_PluginIntf::GMPI_PluginIntf(IGMPI_Unknown* iunknown)
	: GMPI_UnknownIntf(iunknown)
{
	GMPI_TRACE("%p GMPI_PluginIntf::GMPI_PluginIntf(%p)\n", this,
			iunknown);
	m_interface = reinterpret_cast<IGMPI_Plugin*>(iunknown);
}

inline
GMPI_PluginIntf::~GMPI_PluginIntf(void)
{
	GMPI_TRACE("%p GMPI_PluginIntf::~GMPI_PluginIntf()\n", this);
}

#endif /* GMPI_PLUGININTF_HPP_INCLUDED */
