#pragma once

// The decompiled code uses the POSIX <alloca.h> header. MSVC has no such
// header (nor does mingw-w64): alloca lives in <malloc.h> there.
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h>
#ifndef alloca
#define alloca _alloca
#endif
#else
#include_next <alloca.h>
#endif
