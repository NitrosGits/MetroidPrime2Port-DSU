#include "MetroidPrime/ScriptLoaderRel.hpp"

SEmperorIngStage1_FuncPtrs* gLoader_EmperorIngStage1; // Guessed global name.

void SetSEmperorIngStage1_FuncPtrs(SEmperorIngStage1_FuncPtrs* callbacks) {
  gLoader_EmperorIngStage1 = callbacks;
}

// Guessed loader name.
CEntity* RelProxy_LoadEmperorIngStage1(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_EmperorIngStage1 == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_EmperorIngStage1->mLoader(mgr, input, info);
}
