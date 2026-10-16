#include "MetroidPrime/ScriptLoaderRel.hpp"

SIng_FuncPtrs* gLoader_Ing; // Guessed global name.

void SetSIng_FuncPtrs(SIng_FuncPtrs* callbacks) { gLoader_Ing = callbacks; }

// Guessed loader name.
CEntity* RelProxy_LoadIngs(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  // The module has a host body again (src/MetroidPrime/Enemies/PortCIng.cpp), whose RELMain
  // registers the table at port entry, so the null guard the stub needed is gone.
  return gLoader_Ing->mLoader(mgr, input, info);
}
