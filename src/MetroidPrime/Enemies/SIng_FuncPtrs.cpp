#include "MetroidPrime/ScriptLoaderRel.hpp"

SIng_FuncPtrs* gLoader_Ing; // Guessed global name.

void SetSIng_FuncPtrs(SIng_FuncPtrs* callbacks) { gLoader_Ing = callbacks; }

// Guessed loader name.
CEntity* RelProxy_LoadIngs(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_Ing == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_Ing->mLoader(mgr, input, info);
}
