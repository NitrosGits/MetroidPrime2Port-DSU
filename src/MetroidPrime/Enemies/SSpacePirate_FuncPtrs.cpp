#include "MetroidPrime/Enemies/CSpacePirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

SSpacePirate_FuncPtrs* gLoader_SpacePirate; // Guessed global name.

void SetSSpacePirate_FuncPtrs(SSpacePirate_FuncPtrs* callbacks) { gLoader_SpacePirate = callbacks; }

// Guessed loader name.
CEntity* RelProxy_LoadSpacePirate(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  // The module has a host body again (src/MetroidPrime/Enemies/PortCSpacePirate.cpp), whose
  // RELMain registers the table at port entry, so the null guard the stub needed is gone.
  return gLoader_SpacePirate->mLoadSpacePirate(mgr, input, info);
}

// A monolithic DOL links the class's own definition.
#ifndef MONOLITHIC
bool CSpacePirate::AttachActorToPirate(TUniqueId id) {
  return (this->*gLoader_SpacePirate->mAttachActor)(id);
}

void CSpacePirate::DetachActorFromPirate() { (this->*gLoader_SpacePirate->mDetachActor)(); }
#endif
