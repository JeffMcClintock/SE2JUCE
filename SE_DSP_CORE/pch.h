#pragma once

#if defined(_WIN32) // avoid on Apple AND Linux.

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

// Other valid values include 0x0501 for Windows XP, 0x0502 for Windows Server 2003, 0x0600 for Windows Vista, and 0x0601 for Windows 7.
#define _WIN32_WINNT 0x0602
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#if !defined( SE_SUPPORT_MFC )

#include <windows.h>

// MFC substitutes.
#include "./mfc_emulation.h"

#else
// only editor includes MFC

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some std::wstring constructors will be explicit

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>                      // MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>                     // MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#endif

// Jeff: warning C4355: 'this' : used in base member initializer list
#pragma warning( disable : 4355 )

#else
// Apple time!

// MFC substitutes.
#include "./mfc_emulation.h"

#endif // WIN32