#pragma once

// Windows headers - must be first, define macros BEFORE including headers
#define WIN32_LEAN_AND_MEAN

// Include Windows headers first (they define NULL)
#include <windows.h>
#include <commctrl.h>

// Standard C library headers
#include <stddef.h>
#include <stdlib.h>

// ATL basic headers (must be before WTL)
#include <atlbase.h>
#include <atlstr.h>
#include <atltime.h>

// Common Controls v6 Manifest (Required for Group View)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Debug logging macros - only active in Debug builds
// NOTE: These macros expect TCHAR* strings (use _T() macro for literals)
#ifdef _DEBUG
    #define TODO_DEBUG_LOG(msg) ::OutputDebugString(msg)
    #define TODO_DEBUG_LOGF(format, ...) do { \
        TCHAR buf[512]; \
        _stprintf_s(buf, _countof(buf), format, __VA_ARGS__); \
        ::OutputDebugString(buf); \
    } while(0)
#else
    #define TODO_DEBUG_LOG(msg) ((void)0)
    #define TODO_DEBUG_LOGF(format, ...) ((void)0)
#endif

// Always-on logging for errors and critical info
// NOTE: Expects TCHAR* strings (use _T() macro for literals)
#define TODO_LOG(msg) ::OutputDebugString(msg)
#define TODO_LOGF(format, ...) do { \
    TCHAR buf[512]; \
    _stprintf_s(buf, _countof(buf), format, __VA_ARGS__); \
    ::OutputDebugString(buf); \
} while(0)

// SQLite
extern "C" {
#include "sqlite3.h"
}

// WTL headers
#include <WTL/atlapp.h>
#include <WTL/atlframe.h>
#include <WTL/atlctrls.h>
#include <WTL/atldlgs.h>
#include <WTL/atlsplit.h>

// Resource headers
#include "Resource.h"
#include <atlres.h>

// STL headers
#include <vector>
#include <string>
#include <memory>
#include <set>

// Application headers
#include "TodoModel.h"