#include "MetroidPrime/ScriptLoaderRel.hpp"

SCommandoPirate_FuncPtrs* gLoader_CommandoPirate; // Guessed global name.

void SetSCommandoPirate_FuncPtrs(SCommandoPirate_FuncPtrs* callbacks) {
  gLoader_CommandoPirate = callbacks;
}

// Guessed compatibility loader name.
CEntity* RelProxy_LoadCommandPirate(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_CommandoPirate == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_CommandoPirate->mLoader(mgr, input, info);
}
