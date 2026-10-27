#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdio>
#include <io.h>

// The exe is a GUI-subsystem program (no console window of its own); when started from a terminal,
// print to that one, as MP1's PortLogFile::AttachParentConsole. Left alone when stderr already
// goes somewhere (a redirect, a pipe).
void PortAttachParentConsole() {
  const HANDLE error = GetStdHandle(STD_ERROR_HANDLE);
  if ((error != nullptr && error != INVALID_HANDLE_VALUE) || !AttachConsole(ATTACH_PARENT_PROCESS)) {
    return;
  }
  FILE* reopened = nullptr;
  freopen_s(&reopened, "CONOUT$", "w", stdout);
  freopen_s(&reopened, "CONOUT$", "w", stderr);
  SetStdHandle(STD_OUTPUT_HANDLE, reinterpret_cast< HANDLE >(_get_osfhandle(_fileno(stdout))));
  SetStdHandle(STD_ERROR_HANDLE, reinterpret_cast< HANDLE >(_get_osfhandle(_fileno(stderr))));
}
#endif
