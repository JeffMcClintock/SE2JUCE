/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

/*
 * Walk through the GMPI_PLUGIN_PATH and find plugins
 */

#include <stdio.h>
#include <stdlib.h>
#include <gmpi/gmpi.h>
#include <gmpi/lib.h>

static void
FoundAPlugin(const char *path)
{
	printf("%s\n", path);
}

int
main(int argc, char *argv[])
{
	char *path;

	path = getenv("GMPI_PLUGIN_PATH");
	if (path == NULL) {
		if (argc >= 2) {
			path = argv[1];
		} else {
			return -1;
		}
	}
	GMPI_WalkPluginPath(path, FoundAPlugin);

	return 0;
}

