/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "gmpi/lib.h"
#include "gmpi/gmpi.h"

static int
pathIsDir(const char* path)
{
	struct stat s;
	if (stat(path, &s)) {
		return 0;
	}
	return S_ISDIR(s.st_mode);
}

static int
pathIsDll(const char* path)
{
	const char* end = path + (strlen(path) - strlen(GMPI_DLL_EXTENSION));
	return !strcmp(GMPI_DLL_EXTENSION, end);
}

static int
scanDir(const char *dirpath, void (*callback)(const char*))
{
	DIR *dir;
	struct dirent *dirent;
	char *filepath = NULL;
	int count = 0;

	dir = opendir(dirpath);
	if (!dir) {
		return 0;
	}

	while ((dirent = readdir(dir))) {
		int len;

		/* skip dot files */
		if (dirent->d_name[0] == '.') {
			continue;
		}

		/* build a canonical file path */
		len = strlen(dirent->d_name) + strlen(dirpath) + 2;
		filepath = malloc(len);
		snprintf(filepath, len, "%s/%s", dirpath, dirent->d_name);

		/* recursively descend */
		if (pathIsDir(filepath)) {
			count += scanDir(filepath, callback);
		} else if (pathIsDll(filepath)) {
			//FIXME: load the dll and check for GMPI-ness?
			//FIXME: pass the dll handle to callback?
			count++;
			callback(filepath);
		}

		free(filepath);
	}
	closedir(dir);

	return count;
}

int
GMPI_WalkPluginPath(const char* searchpath, void (*callback)(const char*))
{
	char* p0;
	char* p1;
	char* mypath;
	int count = 0;

	/* make a local copy that we can write on */
	mypath = strdup(searchpath);

	/* crawl through the searchpath until the end */
	p0 = p1 = mypath;
	while (*p0) {
		int done = 0;

		/* carve the searchpath into individual elements */
		while (*p1 && *p1 != GMPI_PATH_DELIM) {
			p1++;
		}
		if (*p1 == '\0') {
			done = 1;
		}
		*p1 = '\0';

		/* scan each element for plugins */
		count += scanDir(p0, callback);
		if (!done) {
			p1++;
		}
		p0 = p1;
	}
	free(mypath);

	return count;
}
