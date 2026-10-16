#include "MetroidPrime/ScriptLoaderRel.hpp"

SMinorIng_FuncPtrs* gLoader_MinorIng; // Guessed global name.

void SetSMinorIng_FuncPtrs(SMinorIng_FuncPtrs* callbacks) { gLoader_MinorIng = callbacks; }

// Guessed loader name.
CEntity* RelProxy_LoadMinorIng(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  // The module has a host body again (src/MetroidPrime/Enemies/PortCMinorIng.cpp), whose RELMain
  // registers the table at port entry, so the null guard the stub needed is gone.
  return gLoader_MinorIng->mLoader(mgr, input, info);
}
