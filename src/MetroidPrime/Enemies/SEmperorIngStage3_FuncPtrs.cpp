#include "MetroidPrime/ScriptLoaderRel.hpp"

SEmperorIngStage3_FuncPtrs* gLoader_EmperorIngStage3; // Guessed global name.

void SetSEmperorIngStage3_FuncPtrs(SEmperorIngStage3_FuncPtrs* callbacks) {
  gLoader_EmperorIngStage3 = callbacks;
}

// Guessed loader name.
CEntity* RelProxy_LoadEmperorIngStage3(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_EmperorIngStage3 == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_EmperorIngStage3->mLoader(mgr, input, info);
}
