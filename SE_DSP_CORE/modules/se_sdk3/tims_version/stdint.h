/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_STDINT_H_INCLUDED
#define GMPI_STDINT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
/*
 * Microsft Visual C does not provide <stdint.h> yet
 */
typedef __int8  int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GMPI_STDINT_H_INCLUDED */
