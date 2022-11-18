/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

/*
 * NoopPlugin
 *
 * This is an implementation of a minimal GMPI plugin class in C++.
 */

#include <stdlib.h>
#include <stdio.h>
#include "gmpi/gmpi.h"
#include "gmpi/SdkPlugin.h"
#include "gmpi/SdkFactory.h"
#include "gmpi/Descriptor.h"
#include "gmpi/HostIntf.h"
#include "gmpi/lib.h"
#include "NoopPlugin.h"

/* plugin version info */
#define MY_VMAJ		0
#define MY_VMIN		0
#define MY_VMIC		1
#define MY_VERCODE	GMPI_MKVERSION(MY_VMAJ, MY_VMIN, MY_VMIC)

/* plugin metadata */
#define MY_NAME		"GMPI No-Op Plugin"
#define MY_VERSION	STRINGIFY(MY_VMAJ.MY_VMIN.MY_VMIC)
#define MY_AUTHOR	"Tim Hockin"
#define MY_COPYRIGHT	"Tim Hockin, 2005"
#define MY_NOTES	"This plugin does nothing."
#define MY_HOME		"http://www.gmpi-plugins.org/plugins/noop/"
#define MY_DOCS		"http://www.gmpi-plugins.org/plugins/noop/docs"

/* GMPI versions this plugin supports */
uint32_t MyApiVersions[] = {GMPI_API_VERSION, 0};

static GMPI_Metadata MyMetadata = {
	/* Guid */
	{0x5CC85CAD, 0xDC0C, 0x42C2,
	{0x80, 0xFD, 0xDC, 0x3A, 0xDB, 0xC7, 0x06, 0x41}},

	/* ApiVersions */
	MyApiVersions,

	/* VerCode */
	MY_VERCODE,

	/* Name, Version, Author, Copyright, Notes */
	MY_NAME,
	MY_VERSION,
	MY_AUTHOR,
	MY_COPYRIGHT,
	MY_NOTES,

	/* Home, Docs */
	MY_HOME,
	MY_DOCS,
};

/* the plugin descriptor */
static GMPI_Descriptor MyDescriptor(NoopPlugin::CreateInstance, MyMetadata);

/*
 * IGMPI_Plugin methods
 */

GMPI_Result
NoopPlugin::QueryInterface(const GMPI_Guid& iid, IGMPI_Unknown** iobject)
{
	GMPI_TRACE("%p NoopPlugin::::QueryInterface()\n", this);

	/* if this is not an IID we explicitly handle, pass it on */
	return GMPI_SdkPlugin::QueryInterface(iid, iobject);
}

GMPI_Result
GMPI_STDCALL NoopPlugin::Process(uint32_t count)
{
	GMPI_TRACE("%p NoopPlugin::Process(%d)\n", this, count);
	return GMPI_SUCCESS;
}

GMPI_Result
GMPI_STDCALL NoopPlugin::Placeholder2(int32_t* arg)
{
	GMPI_TRACE("%p NoopPlugin::Placeholder2(%p)\n", this, arg);
	return GMPI_SUCCESS;
}

GMPI_Result
NoopPlugin::CreateInstance(GMPI_HostIntf& host, const char** args,
		IGMPI_Plugin** plugin)
{
	GMPI_TRACE("NoopPlugin::CreateInstance()\n");

	/*
	 * Make a new instance and set the refcount to 1.  Since this
	 * class is derived from GMPI_SdkPlugin (and thereby
	 * IGMPI_Plugin), assigning to the result (type IGMPI_Plugin) from
	 * a NoopPlugin instance is kosher.
	 *
	 * This class doesn't use the args, yet.
	 */
	NoopPlugin *np = new NoopPlugin(host);
	np->AddRef();
	*plugin = np;
	GMPI_TRACE("NoopPlugin::CreateInstance() = %p\n", np);
	return GMPI_SUCCESS;
}

NoopPlugin::NoopPlugin(GMPI_HostIntf& host)
		: GMPI_SdkPlugin(host, MyMetadata)
{
	GMPI_TRACE("%p NoopPlugin::NoopPlugin()\n", this);

	/* make a test call into the host */
	m_host->Placeholder1();
}

NoopPlugin::~NoopPlugin(void)
{
	GMPI_TRACE("%p NoopPlugin::~NoopPlugin()\n", this);
}

/*
 * This function will be called after the DLL is loaded but before any
 * other entry point.  This is where you can do global initialization.
 */
extern "C" GMPI_Result GMPI_STDCALL
GMPI_PreDllHook(void)
{
	/* register this plugin's IID(s) with the factory */
	GMPI_FactoryRegisterIID(MyMetadata.Guid, MyDescriptor);
	return GMPI_SUCCESS;
}
