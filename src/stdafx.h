#pragma once

// Windows headers - must be first
#define WIN32_LEAN_AND_MEAN

// ATL basic headers (must be before WTL)
#include <atlbase.h>
#include <atlstr.h>
#include <atltime.h>

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