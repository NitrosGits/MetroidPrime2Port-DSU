#include "MetroidPrime/ScriptLoaderRel.hpp"

SDigitalGuardian_FuncPtrs* gLoader_DigitalGuardian; // Guessed global name.

void SetSDigitalGuardian_FuncPtrs(SDigitalGuardian_FuncPtrs* callbacks) {
  gLoader_DigitalGuardian = callbacks;
}

// Guessed loader name.
CEntity* RelProxy_LoadDigitalGuardian(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_DigitalGuardian == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_DigitalGuardian->mLoadDigitalGuardian(mgr, input, info);
}

// Guessed loader name.
CEntity* RelProxy_LoadDigitalGuardianHead(CStateManager& mgr, CInputStream& input,
                                          CEntityInfo& info) {
#ifdef TARGET_PC
  // The module has no host body in the port, so its loader table was never
  // registered. Skip the object - the script loader helper tolerates a null entity
  // and skips the rest of its bytes - instead of dereferencing the empty table.
  // Port stand-in; see src/MetroidPrime/PortModuleStubs.cpp.
  if (gLoader_DigitalGuardian == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_DigitalGuardian->mLoadDigitalGuardianHead(mgr, input, info);
}
