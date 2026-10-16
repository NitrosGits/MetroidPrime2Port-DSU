#include "MetroidPrime/ScriptLoaderRel.hpp"

SIngBoostBallGuardian_FuncPtrs* gLoader_IngBoostBallGuardian; // Guessed global name.

void SetSIngBoostBallGuardian_FuncPtrs(SIngBoostBallGuardian_FuncPtrs* callbacks) {
  gLoader_IngBoostBallGuardian = callbacks;
}

// Guessed loader name.
CEntity* RelProxy_LoadIngBoostBallGuardian(CStateManager& mgr, CInputStream& input,
                                           CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_IngBoostBallGuardian == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_IngBoostBallGuardian->mLoader(mgr, input, info);
}
