#include "MetroidPrime/ScriptLoaderRel.hpp"

SSandBoss_FuncPtrs* gLoader_SandBoss; // Guessed global name.

void SetSSandBoss_FuncPtrs(SSandBoss_FuncPtrs* callbacks) { gLoader_SandBoss = callbacks; }

// Guessed loader name.
CEntity* RelProxy_LoadSandBoss(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_SandBoss == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_SandBoss->mLoader(mgr, input, info);
}
