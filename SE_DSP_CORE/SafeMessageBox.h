#pragma once

#ifndef _WIN32
enum app_messagebox_flags { MB_OK, MB_YESNOCANCEL = 0x03, MB_YESNO = 0x04, MB_ICONSTOP = 0x10, MB_ICONEXCLAMATION = 0x30, MB_ICONWARNING = 0x30, MB_ICONINFORMATION = 0x40 };
enum app_messagebox_returns { IDOK=0, IDCANCEL = 2, IDYES = 6, IDNO=7};
#endif

// A safe way to present a messagebox, that will divert to console output on CI (without blocking)
void SafeMessagebox(
    void* hWnd,
    const wchar_t* lpText,
    const wchar_t* lpCaption = L"",
    int uType = 0
);
