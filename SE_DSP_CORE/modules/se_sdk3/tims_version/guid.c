/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */
/*
#include <string.h>
#include <stdio.h>
#include "gmpi.h"
*/
int
GMPI_GuidCompare(const GMPI_Guid* a, const GMPI_Guid* b)
{
	if (a->data1 != b->data1)
		return (a->data1 - b->data1);
	if (a->data2 != b->data2)
		return (a->data2 - b->data2);
	if (a->data3 != b->data3)
		return (a->data3 - b->data3);
	return memcmp(a->data4, b->data4, sizeof(a->data4));
}

int
GMPI_GuidEqual(const GMPI_Guid* a, const GMPI_Guid* b)
{
	return (GMPI_GuidCompare(a, b) == 0);
}
/*
char*
GMPI_GuidString(const GMPI_Guid* guid, char* buffer)
{
	char* p = buffer;
	sprintf(p, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			guid->data1, guid->data2, guid->data3,
			guid->data4[0], guid->data4[1],
			guid->data4[2], guid->data4[3], guid->data4[4],
			guid->data4[5], guid->data4[6], guid->data4[7]);
	return buffer;
}

*/