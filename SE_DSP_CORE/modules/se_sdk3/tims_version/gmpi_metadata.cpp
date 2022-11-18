/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#include <stdio.h>
#include <stdlib.h>
#include "gmpi/gmpi.h"
#include "gmpi/Host.h"
#include "gmpi/UnknownIntf.h"
#include "gmpi/FactoryIntf.h"
#include "gmpi/Guid.h"
#include "gmpi/lib.h"

#define ERROR_CHECK(r, msg)	do { \
	if ((r) != GMPI_SUCCESS) { \
		printf("%s", msg); \
		exit(EXIT_FAILURE); \
	} \
} while(0)

int
main(int argc, char* argv[])
{
	GMPI_DllEntry dllGetFactory;
	GMPI_DllHandle dllHandle;
	char* dllFilename;
	IGMPI_Unknown* iunknown;
	IGMPI_Factory* ifactory;
	GMPI_UnknownIntf* unknown;
	GMPI_FactoryIntf* factory;
	GMPI_Metadata* metadata;
	GMPI_Result r;

	/* handle the commandline */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <plugin>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	dllFilename = argv[1];

	/* load a plugin DLL */
	r = GMPI_DllOpen(&dllHandle, dllFilename);
	ERROR_CHECK(r, "failed to load dll\n");

	/* locate the GMPI entrypoint */
	r = GMPI_DllSymbol(&dllHandle, "GMPI_GetFactory",
			reinterpret_cast<void**>(&dllGetFactory));
	ERROR_CHECK(r, "failed to locate entry point\n");

	/* find the main object */
	r = dllGetFactory(&iunknown);
	ERROR_CHECK(r, "failed to create factory\n");
	unknown = new GMPI_UnknownIntf(iunknown);

	/* ask the unknown for the GMPI Factory interface */
	r = unknown->QueryInterface(GMPI_IID_FACTORY,
			reinterpret_cast<IGMPI_Unknown**>(&ifactory));
	ERROR_CHECK(r, "DLL does not support GMPI Factory v1.0 interface\n");
	factory = new GMPI_FactoryIntf(ifactory);
	delete unknown;

	/* get all the metadata */
	int index = 0;
	while (factory->GetMetadata(index, &metadata) != GMPI_NOSUPPORT) {
		char buffer[37];
		printf("Index %d: {%s}\n", index,
				GMPI_GuidString(&metadata->Guid, buffer));

		int v = 0;
		printf("    ApiVersions: ");
		while (metadata->ApiVersions[v] != 0) {
			uint32_t ver = metadata->ApiVersions[v];
			printf("%d.%d.%d ", GMPI_VER_MAJOR(ver),
					GMPI_VER_MINOR(ver),
					GMPI_VER_MICRO(ver));
			v++;
		}
		printf("\n");

		printf("    VerCode: %d.%d.%d\n",
				GMPI_VER_MAJOR(metadata->VerCode),
				GMPI_VER_MINOR(metadata->VerCode),
				GMPI_VER_MICRO(metadata->VerCode));

		printf("    Name: %s\n", metadata->Name);
		printf("    Version: %s\n", metadata->Version);
		printf("    Author: %s\n", metadata->Author);
		printf("    Copyright: %s\n", metadata->Copyright);
		printf("    Notes: %s\n", metadata->Notes);
		printf("    Home: %s\n", metadata->Home);
		printf("    Docs: %s\n", metadata->Docs);

		index++;
	}

	/* clean up */
	delete factory;

	/* close the DLL */
	r = GMPI_DllClose(&dllHandle);

	return 0;
}
