#include "MetroidPrime/CEntity.hpp"

#include "MetroidPrime/CStateManager.hpp"
#ifdef TARGET_PC
#include <cstdio>
#include <cstdlib>

// MP2_DEBUG_BALL=1: a connection whose message is kSM_Increment is invisible from both ends when
// its target editor id does not resolve - the relay sends it and the receiver never hears it.
static bool PortConnDebug() {
  static const bool enabled = std::getenv("MP2_DEBUG_BALL") != nullptr;
  return enabled;
}
#endif

rstl::vector< SConnection > CEntity::NullConnectionList;

CEntityInfo CEntity::NullEntityInfo =
    CEntityInfo(kInvalidAreaId, NullConnectionList, true, kInvalidEditorId);

CEntityInfo::CEntityInfo(TAreaId aid, const rstl::vector< SConnection >& connections,
                         const bool isActive, TEditorId eid)
: mAreaId(aid)
, mConnections(connections)
, mEditorId(eid)
, mActive(isActive)
, mUpdateWhileOccluded(true)
, mUpdateDuringCinematicSkip(true) {}

CEntity::CEntity(TUniqueId id, const CEntityInfo& info, const rstl::string& name, uint castFlags)
: mAreaId(info.GetAreaId())
, mUniqueId(id)
, mEditorId(info.GetEditorId())
, mConnections(info.GetConnectionList())
, mActive(info.GetActive())
, mNotInArea(mAreaId == kInvalidAreaId)
, mCastFlags(castFlags)
, mUpdateWhileOccluded(info.GetUpdateWhileOccluded())
, mUpdateDuringCinematicSkip(info.GetUpdateDuringCinematicSkip()) {}

CEntity::~CEntity() {}

void CEntity::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_Activate:
    if (!mActive) {
      SetActive(true);
      SendScriptMsgs(kSS_Active, mgr);
    }
    break;
  case kSM_Deactivate:
    if (mActive) {
      SetActive(false);
      SendScriptMsgs(kSS_Inactive, mgr);
    }
    break;
  case kSM_ToggleActive: {
    EScriptObjectMessage next = mActive ? kSM_Deactivate : kSM_Activate;
    CScriptMsg newMsg(msg.GetSenderId(), msg.GetId(), next, msg.GetOriginator(), msg.GetState());
    AcceptScriptMsg(mgr, newMsg);
    break;
  }
  }
}

void CEntity::SendScriptMsgs(EScriptObjectState state, CStateManager& mgr, TUniqueId id,
                             EScriptObjectMessage skipMsg) {
  rstl::vector< SConnection >::const_iterator it = mConnections.begin();
  for (; it != mConnections.end(); ++it) {
    if (it->state == state && it->msg != skipMsg) {
      CStateManager::TIdListResult search = mgr.GetIdListForScript(it->objId);
      CStateManager::TIdList::const_iterator current = search.first;
      CStateManager::TIdList::const_iterator end = search.second;
#ifdef TARGET_PC
      if (PortConnDebug() && (it->msg == kSM_Increment || it->msg == kSM_Action ||
                              it->objId.Value() == 0x0034003f ||
                              GetEditorId().Value() == 0x0034003f)) {
        int matches = 0;
        for (CStateManager::TIdList::const_iterator c = search.first; c != search.second; ++c) {
          ++matches;
        }
        std::fprintf(stderr,
                     "[ball] conn %04x (eid %04x) state %08x msg %08x -> eid %04x: %d match(es)\n",
                     GetUniqueId().Value(), GetEditorId().Value(), unsigned(state),
                     unsigned(it->msg), it->objId.Value(), matches);
      }
#endif
      while (current != end) {
        mgr.SendScriptMsg(CScriptMsg(GetUniqueId(), current->second, it->msg, id, state));
        ++current;
      }
    }
  }
}

void CEntity::PreThink(float dt, CStateManager& mgr) {}

void CEntity::Think(float dt, CStateManager& mgr) {}

void CEntity::SetActive(const bool active) { mActive = active; }

void CEntity::SendActive(CStateManager& mgr, bool active) {
  if (active != GetActive()) {
    mgr.SendScriptMsg(this, GetUniqueId(), active ? kSM_Activate : kSM_Deactivate);
  }
}

TAreaId CEntity::GetAreaIdForPersistence() const { return mNotInArea ? kInvalidAreaId : mAreaId; }

TUniqueId CEntity::FindConnectedObject(const CStateManager& mgr, EScriptObjectState state,
                                       EScriptObjectMessage msg) const {
  for (rstl::vector< SConnection >::const_iterator it = mConnections.begin();
       it != mConnections.end(); ++it) {
    if ((state == kSS_InvalidState || state == it->state) && (msg == kSM_None || msg == it->msg)) {
      CStateManager::TIdListResult ids = mgr.GetIdListForScript(it->objId);
      if (!(ids.first == ids.second)) {
        return ids.first->second;
      }
    }
  }
  return kInvalidUniqueId;
}

TUniqueId CEntity::FindConnectedObject_if(const CStateManager& mgr, EScriptObjectState state,
                                          EScriptObjectMessage msg,
                                          const CValidEntityPredicate& predicate) const {
  for (rstl::vector< SConnection >::const_iterator it = mConnections.begin();
       it != mConnections.end(); ++it) {
    if ((state == kSS_InvalidState || state == it->state) && (msg == kSM_None || msg == it->msg)) {
      CStateManager::TIdListResult ids = mgr.GetIdListForScript(it->objId);
      if (!(ids.first == ids.second)) {
        if (predicate.IsValid(mgr, ids.first->second)) {
          return ids.first->second;
        }
      }
    }
  }
  return kInvalidUniqueId;
}

rstl::vector< TUniqueId > CEntity::FindConnectedObjects(const CStateManager& mgr,
                                                        EScriptObjectState state,
                                                        EScriptObjectMessage msg) const {
  rstl::vector< TUniqueId > result;
  for (rstl::vector< SConnection >::const_iterator it = mConnections.begin();
       it != mConnections.end(); ++it) {
    if ((state == kSS_InvalidState || state == it->state) && (msg == kSM_None || msg == it->msg)) {
      CStateManager::TIdListResult ids = mgr.GetIdListForScript(it->objId);
      if (!(ids.first == ids.second)) {
        result.reserve(result.size() + rstl::distance(ids.first, ids.second));
        for (CStateManager::TIdList::const_iterator current = ids.first; current != ids.second;
             ++current) {
          result.data()[result.mCount++] = current->second;
        }
      }
    }
  }
  return result;
}

rstl::vector< TUniqueId >
CEntity::FindConnectedObjects_if(const CStateManager& mgr, EScriptObjectState state,
                                 EScriptObjectMessage msg,
                                 const CValidEntityPredicate& predicate) const {
  rstl::vector< TUniqueId > result;
  for (rstl::vector< SConnection >::const_iterator it = mConnections.begin();
       it != mConnections.end(); ++it) {
    if ((state == kSS_InvalidState || state == it->state) && (msg == kSM_None || msg == it->msg)) {
      CStateManager::TIdListResult ids = mgr.GetIdListForScript(it->objId);
      if (!(ids.first == ids.second)) {
        result.reserve(result.size() + rstl::distance(ids.first, ids.second));
        for (CStateManager::TIdList::const_iterator current = ids.first; current != ids.second;
             ++current) {
          if (predicate.IsValid(mgr, current->second)) {
            result.data()[result.mCount++] = current->second;
          }
        }
      }
    }
  }
  return result;
}

TUniqueId CEntity::CheckConnectedObject(const CStateManager& mgr, EScriptObjectState state,
                                        EScriptObjectMessage msg) const {
  for (rstl::vector< SConnection >::const_iterator it = mConnections.begin();
       it != mConnections.end(); ++it) {
    if ((state == kSS_InvalidState || state == it->state) && (msg == kSM_None || msg == it->msg)) {
      CStateManager::TIdListResult ids = mgr.GetIdListForScript(it->objId);
      if (!(ids.first == ids.second)) {
        return ids.first->second;
      }
    }
  }
  return kInvalidUniqueId;
}

TUniqueId CEntity::CheckConnectedObject_if(const CStateManager& mgr, EScriptObjectState state,
                                           EScriptObjectMessage msg,
                                           const CValidEntityPredicate& predicate) const {
  for (rstl::vector< SConnection >::const_iterator it = mConnections.begin();
       it != mConnections.end(); ++it) {
    if ((state == kSS_InvalidState || state == it->state) && (msg == kSM_None || msg == it->msg)) {
      CStateManager::TIdListResult ids = mgr.GetIdListForScript(it->objId);
      if (!(ids.first == ids.second)) {
        if (predicate.IsValid(mgr, ids.first->second)) {
          return ids.first->second;
        }
      }
    }
  }
  return kInvalidUniqueId;
}

CValidEntityPredicate::~CValidEntityPredicate() {}

bool CValidEntityPredicate::IsValid(const CStateManager&, TUniqueId) const { return true; }
