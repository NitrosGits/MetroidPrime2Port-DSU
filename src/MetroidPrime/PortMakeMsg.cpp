/**
 * Port-only: the one `MakeMsg` symbol upstream's `src/MetroidPrime/Decode.cpp` does not define.
 *
 * `CMainFlowDtor.cpp` calls `CreateCreateIOWin` by its old placeholder name, `fn_80048EA4`,
 * through `extern "C"` with pointer parameters - the spelling that reproduces retail's call site
 * under mwcceppc (point 1 of that file's notes). `fn_80048EA4` below forwards that call: the first
 * argument is the target (the call site passes 0, `kAMT_IOWinManager`), the next two point at the
 * priority pair, and the last points at the window pointer - retail's `const int&, const int&,
 * CIOWin* const&` passed as the addresses they are.
 *
 * The `MakeMsg` factories and accessors themselves (`CreateCreateIOWin`, `CreateUserInput`,
 * `CreateControllerStatus`, `GetParm*`) used to be copied here; `Decode.cpp` is now in
 * files.cmake and defines them, so the copies were deleted. This file is not in configure.py,
 * so it cannot affect main.dol.
 */
#include "MetroidPrime/Decode.hpp"

extern "C" CArchitectureMessage fn_80048EA4(const char* target, const int* pmin, const int* pmax,
                                            void* iowin) {
  return MakeMsg::CreateCreateIOWin(static_cast< EArchMsgTarget >(reinterpret_cast< uintptr_t >(target)),
                                    *pmin, *pmax, *static_cast< CIOWin* const* >(iowin));
}
