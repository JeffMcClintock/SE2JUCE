/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_GUID_HPP_INCLUDED
#define GMPI_GUID_HPP_INCLUDED

#include "gmpi.h" /* JM-MOD */
#include "lib.h" /* JM-MOD */
#include <iostream>

inline bool
operator==(const GMPI_Guid& left, const GMPI_Guid& right)
{
	return (GMPI_GuidEqual(&left, &right) != 0);
}

inline bool
operator<(const GMPI_Guid& left, const GMPI_Guid& right)
{
	return (GMPI_GuidCompare(&left, &right) < 0);
}

inline std::ostream&
operator<<(std::ostream& stream, const GMPI_Guid& guid)
{
	char buffer[40];
	stream << GMPI_GuidString(&guid, buffer);
	return stream;
}

#endif /* GMPI_GUID_HPP_INCLUDED */
