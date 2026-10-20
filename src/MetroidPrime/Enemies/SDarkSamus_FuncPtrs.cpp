#include "MetroidPrime/ScriptLoaderRel.hpp"

SDarkSamus_FuncPtrs* gLoader_DarkSamus; // Guessed global name.

void SetSDarkSamus_FuncPtrs(SDarkSamus_FuncPtrs* callbacks) { gLoader_DarkSamus = callbacks; }

// Guessed loader name.
CEntity* RelProxy_LoadDarkSamus(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has a host body (src/MetroidPrime/Enemies/PortCDarkSamus.cpp), whose RELMain
  // registers the table at port entry, so the null guard the stub needed is gone.
#endif
  return gLoader_DarkSamus->mLoader(mgr, input, info);
}
