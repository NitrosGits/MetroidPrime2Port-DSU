#include "MetroidPrime/ScriptLoaderRel.hpp"

SMinorIng_FuncPtrs* gLoader_MinorIng; // Guessed global name.

void SetSMinorIng_FuncPtrs(SMinorIng_FuncPtrs* callbacks) { gLoader_MinorIng = callbacks; }

// Guessed loader name.
CEntity* RelProxy_LoadMinorIng(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_MinorIng == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_MinorIng->mLoader(mgr, input, info);
}
