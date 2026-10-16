#include "MetroidPrime/ScriptLoaderRel.hpp"

SSwampBossStage1_FuncPtrs* gLoader_SwampBossStage1; // Guessed global name.

void SetSSwampBossStage1_FuncPtrs(SSwampBossStage1_FuncPtrs* callbacks) {
  gLoader_SwampBossStage1 = callbacks;
}

// Guessed loader name.
CEntity* RelProxy_LoadSwampBossStage1(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_SwampBossStage1 == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_SwampBossStage1->mLoader(mgr, input, info);
}
