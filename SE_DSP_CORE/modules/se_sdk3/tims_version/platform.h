/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_PLATFORM_H_INCLUDED
#define GMPI_PLATFORM_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h" /* JM-MOD */

/*
 * These are the known platforms
 */
#if defined(WIN32)
#define GMPI_PLATFORM_WIN32	1
#define GMPI_STDCALL		_stdcall
#define GMPI_PATH_DELIM		';'
#define GMPI_DLL_EXTENSION	".dll"
#define GMPI_EXE_EXTENSION	".exe"
#endif


#if defined(MACOSX)
#define GMPI_PLATFORM_MACOSX	1
#define GMPI_STDCALL		
#define GMPI_PATH_DELIM		':'
#define GMPI_DLL_EXTENSION	".dylib"
#define GMPI_EXE_EXTENSION	".app"
#endif


#if defined(__linux__)
#define GMPI_PLATFORM_LINUX	1
#if defined(__i386__)
#define GMPI_STDCALL		__attribute__((stdcall))
#else
#define GMPI_STDCALL		
#endif
#define GMPI_PATH_DELIM		':'
#define GMPI_DLL_EXTENSION	".so"
#define GMPI_EXE_EXTENSION	""
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GMPI_PLATFORM_H_INCLUDED */
