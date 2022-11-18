/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_SDKFACTORY_HPP_INCLUDED
#define GMPI_SDKFACTORY_HPP_INCLUDED

#include "gmpi.h"
#include "Guid.h"
#include "Descriptor.h"

/* register a plugin with the factory */
extern GMPI_Result GMPI_FactoryRegisterIID(const wchar_t *unique_id,
		GMPI_Descriptor& descriptor);

#endif /* GMPI_SDKFACTORY_HPP_INCLUDED */
