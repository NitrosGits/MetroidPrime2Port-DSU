// Port-only: the PortLinkStubs.cpp entries the game still references, as real C++ definitions for
// MSVC and Apple. That file names its stubs with asm() labels spelled as Itanium/ELF symbols,
// which an MSVC-mangled (or Mach-O underscore-prefixed) reference never matches. ELF and MinGW
// builds keep using the generated stubs; this unit is empty there.

#if defined(_MSC_VER) || defined(__APPLE__)

#include "Kyoto/Basics/COsContext.hpp"
#include "MetroidPrime/CSaveRegion.hpp"

class CCubeRenderer;
class IController;

// Startup.cpp (unbuilt) defines these; main.cpp sets the pointers.
void* CSaveRegion::mSaveBuffer;
const void* CSaveRegion::mNonVolatileSettingsBuf;
CCubeRenderer* gpRender;
IController* gpController;

// Retail reads the dev-kit reset keys here; no key is ever down on the host.
COsKeyState COsContext::GetOsKeyState(int key) const {
  return COsKeyState(key, false, false, false, false);
}

#endif
