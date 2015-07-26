// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <atlstr.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifndef noexcept
#define noexcept throw()
#include <scope_guard.h>
#include <unique_resource.h>
#undef noexcept
#else
#include <scope_guard.h>
#include <unique_resource.h>
#endif

#include <boost/core/noncopyable.hpp>

#include "Logging.h"
using namespace Logging;

#include "SmartPointers.h"