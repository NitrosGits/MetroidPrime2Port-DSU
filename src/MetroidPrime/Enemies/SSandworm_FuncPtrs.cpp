#include "MetroidPrime/Enemies/CSandworm.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

SSandworm_FuncPtrs* gLoader_Sandworm; // Guessed name.

void SetSSandworm_FuncPtrs(SSandworm_FuncPtrs* callbacks) { gLoader_Sandworm = callbacks; }

CEntity* RelProxy_LoadSandworm(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_Sandworm == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_Sandworm->mLoadSandworm(mgr, input, info);
}

CVector3f GetSandwormRadarPointPosition(const CSandworm* sandworm, int index) {
  return (sandworm->*gLoader_Sandworm->mGetRadarPointPosition)(index);
}

int GetSandwormRadarPointCount(const CSandworm* sandworm) {
  return (sandworm->*gLoader_Sandworm->mGetRadarPointCount)();
}
