#include "MetroidPrime/ScriptLoaderRel.hpp"

SElitePirate_FuncPtrs* gLoader_ElitePirate; // Guessed global name.

void SetSElitePirate_FuncPtrs(SElitePirate_FuncPtrs* callbacks) { gLoader_ElitePirate = callbacks; }

// Guessed loader name.
CEntity* RelProxy_LoadElitePirate(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_ElitePirate == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_ElitePirate->mLoader(mgr, input, info);
}
