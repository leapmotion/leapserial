// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once

// We want zlib to provide us with a const flag for its struct declarations
#define ZLIB_CONST

#if _WIN32
#define _CRT_NONSTDC_NO_DEPRECATE
#define NOMINMAX

#include <Windows.h>

#include <vector>
#include <fstream>
#include <iomanip>
#include <map>
#include <string>
#include <utility>
#include <cassert>

#endif // _WIN32
