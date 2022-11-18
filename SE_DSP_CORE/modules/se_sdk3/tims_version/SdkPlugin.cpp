/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

/*
 * GMPI_SdkPlugin
 *
 * This file provides a base class which implements the IGMPI_Plugin
 * interface.
 */

#include <stdlib.h>
#include "gmpi.h" /* JM-MOD */
#include "SdkPlugin.h" /* JM-MOD */
#include "HostIntf.h" /* JM-MOD */
#include "Guid.h" /* JM-MOD */
#include "lib.h" /* JM-MOD */

/*
 * C to C++ thunks
 */

static GMPI_Result GMPI_STDCALL
GP_QueryInterface(IGMPI_Plugin* igp, const GMPI_Guid* iid,
		IGMPI_Unknown** iobject)
{
	GMPI_SdkPlugin* plugin = static_cast<GMPI_SdkPlugin*>(igp);
	return plugin->QueryInterface(*iid, iobject);
}

static int32_t GMPI_STDCALL
GP_AddRef(IGMPI_Plugin* igp)
{
	GMPI_SdkPlugin* plugin = static_cast<GMPI_SdkPlugin*>(igp);
	return plugin->AddRef();
}

static int32_t GMPI_STDCALL
GP_Release(IGMPI_Plugin* igp)
{
	GMPI_SdkPlugin* plugin = static_cast<GMPI_SdkPlugin*>(igp);
	return plugin->Release();
}

static GMPI_Result GMPI_STDCALL
GP_Process(IGMPI_Plugin* igp, uint32_t count)
{
	GMPI_SdkPlugin* plugin = static_cast<GMPI_SdkPlugin*>(igp);
	return plugin->Process(count);
}

static GMPI_Result GMPI_STDCALL
GP_GetPinMetadata(IGMPI_Plugin* igp, int32_t index, GMPI_PinMetadata* metadata)
{
	GMPI_SdkPlugin* plugin = static_cast<GMPI_SdkPlugin*>(igp);
	return plugin->GetPinMetadata( index, metadata );
}

/* This defines the plugin methods interface for the C linkage */
static IGMPI_PluginMethods GP_Methods = {
	GP_QueryInterface,
	GP_AddRef,
	GP_Release,
	GP_Process,
	GP_GetPinMetadata,
};


/*
 * Plugin methods
 */

GMPI_Result
GMPI_SdkPlugin::QueryInterface(const GMPI_Guid& iid, IGMPI_Unknown** iobject)
{
	GMPI_TRACE("%p GMPI_SdkPlugin::QueryInterface()\n", this);

	/*
	 * For SDK plugins, the root object (IGMPI_Unknown) is the same
	 * as the plugin object (IGMPI_Plugin).  It's simpler that way.
	 *
	 * By spec, the interface pointer for an object's own GUID is the
	 * same as the instance's root object pointer, which is the same
	 * as the instance's unknown interface pointer.
	 */
	if (iid == GMPI_IID_UNKNOWN || iid == GMPI_IID_PLUGIN ||
			iid == m_metadata->Guid) {
		IGMPI_Plugin* iplugin = static_cast<IGMPI_Plugin*>(this);
		*iobject = reinterpret_cast<IGMPI_Unknown*>(iplugin);
		AddRef();
		return GMPI_SUCCESS;
	}

	*iobject = NULL;
	return GMPI_NOSUPPORT;
}

int32_t
GMPI_SdkPlugin::AddRef(void)
{
	m_refcount++;
	GMPI_TRACE("%p GMPI_SdkPlugin::AddRef() = %d\n",
			this, m_refcount);
	return m_refcount;
}

int32_t
GMPI_SdkPlugin::Release(void)
{
	m_refcount--;
	GMPI_TRACE("%p GMPI_SdkPlugin::Release() = %d\n",
			this, m_refcount);

	/*
	 * For SDK plugins, we assume they are dynamically created.  If
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
GMPI_SdkPlugin::Process(uint32_t count)
{
	GMPI_TRACE("%p GMPI_SdkPlugin::Process(%d)\n", this, count);
	return GMPI_FAIL;
}

GMPI_Result GMPI_SdkPlugin::GetPinMetadata(int32_t index, GMPI_PinMetadata* metadata)
{
	GMPI_TRACE("%p GMPI_SdkPlugin::GetPinMetadata(%p)\n", this, index);
	return GMPI_FAIL;
}

GMPI_SdkPlugin::GMPI_SdkPlugin(GMPI_HostIntf& host, GMPI_Metadata& metadata)
{
	GMPI_TRACE("%p GMPI_SdkPlugin::GMPI_SdkPlugin()\n", this);
	IGMPI_Plugin::methods = &GP_Methods;
	m_refcount = 0;
	m_host = &host;
	m_metadata = &metadata;
}

GMPI_SdkPlugin::~GMPI_SdkPlugin(void)
{
	GMPI_TRACE("%p GMPI_SdkPlugin::~GMPI_SdkPlugin()\n", this);
	delete m_host;
}
