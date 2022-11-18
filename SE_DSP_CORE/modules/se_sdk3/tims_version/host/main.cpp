/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#include <stdio.h>
#include <stdlib.h>
#include "gmpi.h" /* JM-MOD */
#include "Host.h" /* JM-MOD */
#include "UnknownIntf.h" /* JM-MOD */
#include "PluginIntf.h" /* JM-MOD */
#include "FactoryIntf.h" /* JM-MOD */
#include "Guid.h" /* JM-MOD */
#include "lib.h" /* JM-MOD */
#include "ExampleHost.h" /* JM-MOD */

using namespace std;

#define ERROR_CHECK(r, msg)	do { \
	if ((r) != GMPI_SUCCESS) { \
		printf("%s", msg); \
		exit(EXIT_FAILURE); \
	} \
} while(0)

/* use a global host object, for now */
static ExampleHost theHost;

/* use a global empty args list, for now */
static const char *theArgs[2] = { NULL, NULL };

int
main(int argc, char* argv[])
{
	GMPI_DllEntry dllGetFactory;
	GMPI_DllHandle dllHandle;
	char* dllFilename;
	GMPI_FactoryIntf *factory;
	GMPI_PluginIntf *plugin;
	GMPI_Result r;

	/*
	 * assuming Plugin ID sourced elsewhere (from metadata file)
	 * format of plugin unique ID undecided. for now using GUID
	 * {5CC85CAD-DC0C-42c2-80FD-DC3ADBC70641}
	 */
	static const GMPI_Guid IID_NoopPlugin = {
		0x5cc85cad, 0xdc0c, 0x42c2,
		{0x80, 0xfd, 0xdc, 0x3a, 0xdb, 0xc7, 0x06, 0x41}
	};

	/* handle the commandline */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <plugin>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	dllFilename = argv[1];

	/* theHost is static - add a reference so it never hits 0 */
	theHost.AddRef();

	/* load a plugin DLL */
	r = GMPI_DllOpen(&dllHandle, dllFilename);
	ERROR_CHECK(r, "failed to load dll\n");

	/* locate the GMPI entrypoint */
	r = GMPI_DllSymbol(&dllHandle, "GMPI_GetFactory",
			reinterpret_cast<void**>(&dllGetFactory));
	ERROR_CHECK(r, "failed to locate entry point\n");

	{
		/* find the main object */
		IGMPI_Unknown *iunknown;
		r = dllGetFactory(&iunknown);
		ERROR_CHECK(r, "failed to create factory\n");

		/* make a local UnknownIntf */
		GMPI_UnknownIntf unknown(iunknown);

		/* ask the main object for the GMPI Factory interface */
		IGMPI_Unknown *ifactory;
		r = unknown.QueryInterface(GMPI_IID_FACTORY, &ifactory);
		ERROR_CHECK(r, "DLL does not support IGMPI_Factory interface\n");
		factory = new GMPI_FactoryIntf(ifactory);
	}

	{
		/* add a reference for the new plugin */
		theHost.AddRef();

		/* set up the args array */
		theArgs[0] = dllFilename;

		/* ask the factory to instantiate its first avail plugin */

		/* query plugin unique ID */
		GMPI_Metadata *md;
		r = factory->GetMetadata(0, &md);
		ERROR_CHECK(r, "failed to query plugin metadata\n");

		/* use ID to instantiate plugin */
		IGMPI_Unknown *iunknown;
		r = factory->CreateInstance(md->Guid, theHost, theArgs,
			&iunknown);
		ERROR_CHECK(r, "failed to create plugin\n");

		/* make a local UnknownIntf */
		GMPI_UnknownIntf unknown(iunknown);

		/* ask the object for the GMPI Plugin interface */
		IGMPI_Unknown *iplugin;
		r = unknown.QueryInterface(GMPI_IID_PLUGIN, &iplugin);
		ERROR_CHECK(r, "plugin does not support IGMPI_Plugin interface\n");
		plugin = new GMPI_PluginIntf(iplugin);
	}

	/* query the pin data */
	int idx = 0;
	GMPI_PinMetadata mp;
	do {
		// set metadata to default values
		memset( &mp, 0, sizeof(mp) );

		r = plugin->GetPinMetadata( idx, &mp );

		if( r == GMPI_SUCCESS )
		{
			fprintf(stderr, "Plug number %d, name=%s\n", idx, mp.name);
		}
		idx++;
	} while( r == GMPI_SUCCESS );


	/* run the process loop */
	for (int i = 0; i < 100; i++) {
		plugin->Process(1024);
	}

	/* clean up */
	delete factory;
	delete plugin;

	//FIXME: unwind any errors

	/* close the DLL */
	r = GMPI_DllClose(&dllHandle);

	return 0;
}

