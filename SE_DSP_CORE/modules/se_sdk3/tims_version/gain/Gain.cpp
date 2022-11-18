/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

/*
 * Gain
 *
 * This is an implementation of a minimal GMPI plugin class in C++.
 */

#include <stdlib.h>
#include <stdio.h>
#include "gmpi.h"
#include "SdkPlugin.h"
#include "SdkFactory.h"
#include "Descriptor.h"
#include "HostIntf.h"
#include "lib.h"
#include "Gain.h"

/* plugin version info */
#define MY_VMAJ		0
#define MY_VMIN		0
#define MY_VMIC		1
#define MY_VERCODE	GMPI_MKVERSION(MY_VMAJ, MY_VMIN, MY_VMIC)

/* plugin metadata */
#define MY_NAME		"GMPI Gain Plugin"
#define MY_VERSION	STRINGIFY(MY_VMAJ.MY_VMIN.MY_VMIC)
#define MY_AUTHOR	"Jeff McClintock"
#define MY_COPYRIGHT	"GMPI Working Group"
#define MY_NOTES	"This plugin does almost nothing."
#define MY_HOME		""
#define MY_DOCS		""

/* GMPI versions this plugin supports */
uint32_t MyApiVersions[] = {GMPI_API_VERSION, 0};

static GMPI_Metadata MyMetadata = {
	/* Guid */
	{0x5CC85CAD, 0xDC0C, 0x42C2,
	{0x80, 0xFD, 0xDC, 0x3A, 0xDB, 0xC7, 0x06, 0x42}},

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
static GMPI_Descriptor MyDescriptor(Gain::CreateInstance, MyMetadata);

/*
 * IGMPI_Plugin methods
 */

GMPI_Result
Gain::QueryInterface(const GMPI_Guid& iid, IGMPI_Unknown** interface)
{
	GMPI_TRACE("%p Gain::::QueryInterface()\n", this);

	/* if this is not an IID we explicitly handle, pass it on */
	return GMPI_SdkPlugin::QueryInterface(iid, interface);
}

GMPI_Result
Gain::Process(int32_t arg)
{
	GMPI_TRACE("%p Gain::Process(%d)\n", this, arg);
	return GMPI_SUCCESS;
}

GMPI_Result Gain::GetPinMetadata(int32_t index, GMPI_PinMetadata* metadata)
{
	GMPI_TRACE("%p Gain::GetPinMetadata(%p)\n", this, index);

	switch ( index )
	{
	case 0:
		metadata->name		= "Input";
		metadata->direction	= GMPI_IN;
		metadata->datatype	= GMPI_AUDIO;
		break;

	case 1:
		metadata->name		= "Output";
		metadata->direction	= GMPI_OUT;
		metadata->datatype	= GMPI_AUDIO;
		break;

	default:
		return GMPI_FAIL;
	};

	return GMPI_SUCCESS;
}

GMPI_Result
Gain::CreateInstance(GMPI_HostIntf& host, const char** args,
		IGMPI_Plugin** plugin)
{
	GMPI_TRACE("Gain::CreateInstance()\n");

	/*
	 * Make a new instance and set the refcount to 1.  Since this
	 * class is derived from GMPI_SdkPlugin (and thereby
	 * IGMPI_Plugin), assigning to the result (type IGMPI_Plugin) from
	 * a Gain instance is kosher.
	 *
	 * This class doesn't use the args, yet.
	 */
	Gain *np = new Gain(host);
	np->AddRef();
	*plugin = np;
	GMPI_TRACE("Gain::CreateInstance() = %p\n", np);
	return GMPI_SUCCESS;
}

Gain::Gain(GMPI_HostIntf& host)
		: GMPI_SdkPlugin(host, MyMetadata)
{
	GMPI_TRACE("%p Gain::Gain()\n", this);

	/* make a test call into the host */
	m_host->Placeholder1();
}

Gain::~Gain(void)
{
	GMPI_TRACE("%p Gain::~Gain()\n", this);
}

/* register this plugin with factory */
namespace
{
	GMPI_Result r = GMPI_FactoryRegisterIID(MyMetadata.Guid, MyDescriptor);
}