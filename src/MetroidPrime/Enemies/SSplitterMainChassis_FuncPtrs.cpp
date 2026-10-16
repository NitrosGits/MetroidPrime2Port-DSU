#include "MetroidPrime/Enemies/CSplitterMainChassis.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

SSplitterMainChassis_FuncPtrs* gLoader_Splitter; // Guessed name.

void SetSSplitterMainChassis_FuncPtrs(SSplitterMainChassis_FuncPtrs* callbacks) {
  gLoader_Splitter = callbacks;
}

// This module is `Splitter` in tools/modules_port.txt, which the port still registers through the
// shared entry-point stub (PortModuleStubs.cpp): its table is never registered, so every call
// through it has to tolerate a null. Noticing that the module's name (`Splitter`) and its class
// (`CSplitterMainChassis`) differ is what this file's guard depends on - a sweep died here in
// RelProxy_LoadSplitterMainChassis+0xe, dereferencing the null table while loading an area.
CEntity* RelProxy_LoadSplitterMainChassis(CStateManager& mgr, CInputStream& input,
                                          CEntityInfo& info) {
#ifdef TARGET_PC
  if (gLoader_Splitter == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_Splitter->mLoadMainChassis(mgr, input, info);
}

CEntity* RelProxy_LoadSplitterCommandModule(CStateManager& mgr, CInputStream& input,
                                            CEntityInfo& info) {
#ifdef TARGET_PC
  if (gLoader_Splitter == nullptr) {
    return nullptr;
  }
#endif
  return gLoader_Splitter->mLoadCommandModule(mgr, input, info);
}

// A monolithic DOL links the class's own definition.
#ifndef MONOLITHIC
void CSplitterMainChassis::AutoDestruct(float time) {
#ifdef TARGET_PC
  if (gLoader_Splitter == nullptr) {
    return;
  }
#endif
  (this->*gLoader_Splitter->mAutoDestruct)(time);
}
#endif
