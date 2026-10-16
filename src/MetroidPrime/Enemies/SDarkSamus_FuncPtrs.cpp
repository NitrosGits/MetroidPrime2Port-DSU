#include "MetroidPrime/ScriptLoaderRel.hpp"

SDarkSamus_FuncPtrs* gLoader_DarkSamus; // Guessed global name.

void SetSDarkSamus_FuncPtrs(SDarkSamus_FuncPtrs* callbacks) { gLoader_DarkSamus = callbacks; }

// Guessed loader name.
CEntity* RelProxy_LoadDarkSamus(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_DarkSamus == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_DarkSamus->mLoader(mgr, input, info);
}
