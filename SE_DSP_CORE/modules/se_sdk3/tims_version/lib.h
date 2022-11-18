/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_LIB_H_INCLUDED
#define GMPI_LIB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "gmpi.h" /* JM-MOD */
#include "stdint.h" /* JM-MOD */

/*
 * Helpers for tracing execution
 */

#define GMPI_TRACE_CALLS 1

#ifndef GMPI_TRACE_PREFIX
#define GMPI_TRACE_PREFIX "TRACE"
#endif

#ifdef GMPI_TRACE_CALLS
#include <stdio.h>
#include <stdarg.h>
inline void GMPI_TRACE(const char *fmt, ...)
{
	va_list args;
	printf("%s: ", GMPI_TRACE_PREFIX);
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}
#else
static inline void GMPI_TRACE(const char *fmt, ...) {}
#endif


/*
 * GMPI_DllHandle
 *
 * An opaque handle for a dynamically loaded file.
 */

typedef void* GMPI_DllHandle;

/*
 * GMPI_DllOpen()
 *
 * Open and load a dynamically loadable file. If the DLL is successfully
 * loaded, this function will look for the symbol "GMPI_PreDllHook" in the
 * DLL's symbol table.  If found, this function will be invoked.  If the
 * GMPI_PreDllHook fails, the library will attempt to unload the DLL.  The
 * the DLL's "GMPI_PostDllHook" will not be invoked.  The GMPI_PreDllHook
 * has the signature:
 * 	GMPI_Result GMPI_PreDllHook(void);
 *
 * In:
 *   filename: The file name of the DLL file.
 *
 * Out:
 *   handle: A pointer to an opaque GMPI_DllHandle.  The handle will be
 *   	used by all the GMPI_Dll functions.
 *
 * Return:
 *   GMPI_SUCCESS if everything succeeds.  If this function fails, it will
 *   return GMPI_FAIL or some other error code indicating the failure
 *   mode.
 */
extern GMPI_Result GMPI_DllOpen(GMPI_DllHandle* handle, const char* filename);

/*
 * GMPI_DllSymbol()
 *
 * Look for the specified symbol in the symbol table of the DLL
 * represented by the handle.
 *
 * In:
 *   handle: The opaque GMPI_DllHandle for the requested DLL.
 *   symbol: The symbol to lookup.
 *
 * Out:
 *   result: A pointer to a void pointer which will be set to point at
 *   	the requested symbol, if found.
 *
 * Return:
 *   GMPI_SUCCESS if the requested symbol was found, GMPI_FAIL if the
 *   symbol was not found.
 */
extern GMPI_Result GMPI_DllSymbol(GMPI_DllHandle* handle, const char* symbol,
		void** result);

/*
 * GMPI_DllClose()
 *
 * Release and possibly unload the specified DLL. Before the DLL is
 * released, this function will look for the symbol "GMPI_PostDllHook" in
 * the DLL's symbol table.  If found, this function will be invoked.  The
 * DLL will be released, and if there are no more handles open to it, the
 * DLL will be unloaded.  After calling this function, the handle is no
 * longer valid and must not be used.  The GMPI_PostDllHook has the
 * signature:
 * 	void GMPI_PostDllHook(void);
 *
 * In:
 *   handle: The opaque GMPI_DllHandle for the requested DLL.
 *
 * Out:
 *   None.
 *
 * Return:
 *   GMPI_SUCCESS if everything succeeds.  If this function fails, it will
 *   return GMPI_FAIL or some other error code indicating the failure
 *   mode.
 */
extern GMPI_Result GMPI_DllClose(GMPI_DllHandle* handle);


/*
 * GMPI_GuidCompare()
 *
 * Compare two GUIDs.
 *
 * In:
 *   a: The first GUID.
 *   b: The second GUID.
 *
 * Out:
 *   None.
 *
 * Return:
 *   Zero if the GUIDs are equal.  If the GUIDs are not equal, this
 *   function will return a number less than zero if the first GUID is
 *   less than the second, or a number greater than zero if the first GUID
 *   is greater than the second.
 */
extern int GMPI_GuidCompare(const GMPI_Guid* a, const GMPI_Guid* b);

/*
 * GMPI_GuidEqual()
 *
 * Compare two GUIDs and determine if they are the same GUID.
 *
 * In:
 *   a: The first GUID.
 *   b: The second GUID.
 *
 * Out:
 *   None.
 *
 * Return:
 *   Zero if the GUIDs are equal, non-zero if the GUIDs are not equal.
 */
extern int GMPI_GuidEqual(const GMPI_Guid* a, const GMPI_Guid* b);

/*
 * GMPI_GuidString()
 *
 * Print the canonical string representation of a GUID into a buffer (not
 * including the curly braces which typically surround a GUID string).
 *
 * In:
 *   guid: The GUID to render.
 *   buffer: The buffer into which to render.  This must be at least 37
 *   	bytes (32 characters, 4 dashes, and NUL).
 *
 * Out:
 *   None.
 *
 * Return:
 *   A pointer to the buffer.
 */
extern char* GMPI_GuidString(const GMPI_Guid* guid, char* buffer);


/*
 * GMPI_WalkPluginPath()
 *
 * Enumerate all GMPI plugins in the specified search path.  The search
 * path will be split into sections, delimited by GMPI_PATH_DELIM.  Each
 * section will be recursively scanned for GMPI plugin DLLs.  Whenever a
 * GMPI DLL is found, the callback will be called, with the full file path
 * to the DLL file as the argument.
 *
 * In:
 *   searchpath: The delimited list of directories to search.
 *   callback: The function to call when a plugin DLL is found.
 *
 * Out:
 *   None.
 *
 * Return:
 *   The total number of plugin DLLs found.
 */
extern int GMPI_WalkPluginPath(const char* searchpath,
		void (*callback)(const char*));

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GMPI_LIB_H_INCLUDED */
