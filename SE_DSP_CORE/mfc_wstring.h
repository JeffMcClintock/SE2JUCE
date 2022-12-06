#pragma once
#include <string>

// No MFC in plugin.
// don't want std::wstring clients depending on MFC
#if defined( SE_SUPPORT_MFC )
CArchive& AFXAPI operator>>( CArchive& ar, std::wstring& pOb );
CArchive& AFXAPI operator<<( CArchive& ar, std::wstring& pOb );
CArchive& AFXAPI operator>>( CArchive& ar, std::string& pOb );
CArchive& AFXAPI operator<<( CArchive& ar, std::string& pOb );

std::wstring CStringToWstring(const CString& s);
CString WstringToCString(std::wstring s);

#endif
