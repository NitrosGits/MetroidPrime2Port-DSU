
#include "MetroidPrime/ScriptObjects/CScriptCannonBall.hpp"

#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CMorphBall.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader/SLdrCannonBall.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptEffect.hpp"
#include "MetroidPrime/TCastTo.hpp"
#ifdef TARGET_PC
#include <cstdio>
#include <cstdlib>

// MP2_DEBUG_BALL=1: the cannon ball is a script flag, not an inventory item, and a launch that
// clears it early leaves the ball with no damage pass (it bounces off the wall instead of
// breaking it). This says when the flag goes up and why it comes down.
static bool PortBallDebug() {
  static const bool enabled = std::getenv("MP2_DEBUG_BALL") != nullptr;
  return enabled;
}
#endif

// @ fn_57_110C
CScriptCannonBall::CScriptCannonBall(TUniqueId uid, const rstl::string& name,
                                     const CEntityInfo& info, const CTransform4f& xf,
                                     CAssetId effect)

: CActor(uid, name, info, 0, xf, CModelData::None(), CMaterialList(), CActorParameters::None(),
         kInvalidUniqueId)
, m_effect(effect) {}

// @ fn_57_1054
CScriptCannonBall::~CScriptCannonBall() {}

void CScriptCannonBall::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
#ifdef TARGET_PC
  if (PortBallDebug()) {
    const CEntity* origin = mgr.GetObjectById(msg.GetOriginator());
    std::fprintf(stderr, "[ball] cannonscript %04x message %08x from %04x (%s)\n",
                 GetUniqueId().Value(), unsigned(msg.GetMessage()), msg.GetOriginator().Value(),
                 origin != nullptr ? "object" : "none");
  }
#endif
  switch (msg.GetMessage()) {

  case kSM_Increment: {
    if (CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(msg.GetOriginator()))) {
      CMorphBall* morph = player->GetMorphBall();
      CTransform4f xf = morph->GetSurfaceToWorld();
      player->SetTransform(
          CTransform4f(xf.BuildMatrix3f(), player->GetTranslation())); // todo use position
      morph->SwitchToTire();
      m_fields[player->GetPlayerIndex()].OnIncrementMsg(mgr, true);
    }
    break;
  }

  case kSM_Create: {
    for (int playerIndex = 0; playerIndex < mgr.GetNumPlayers(); ++playerIndex) {
      TUniqueId id = mgr.AllocateUniqueId();

      CLightParameters lParams = CLightParameters::None();
      CGameSplineDesc spline(SLdrSpline(), CMotionSpline::kST_Bezier, 1.0f, false);

      CScriptEffect* newEffect =
          new CScriptEffect(id, rstl::string_l("CannonBall Effect"),
                            CEntityInfo(GetCurrentAreaId(), rstl::vector< SConnection >(), true),
                            CTransform4f::Identity(), CVector3f::One(), m_effect, 1, 0, 0, 0, 1.0f,
                            1.0f, 0.0f, 0.0f, false, 1.0f, 2.0f, 1.0f, true, true, true, lParams,
                            false, spline, false, false, false, CScriptEffect::kRO_Normal);
      newEffect->SetNextDrawNode(mgr.GetPlayer(playerIndex)->GetUniqueId());
      mgr.AddObject(newEffect);

      m_fields.push_back(TrackedShot(id, false));
    }
    break;
  }

  case kSM_Delete: {
    for (int i = 0; i < m_fields.size(); ++i) {
      m_fields[i].FreeScriptObject(mgr);
    }
    m_fields.clear();
    break;
  }

  default:
    break;
  }
}

// @ fn_57_694
void CScriptCannonBall::Think(float dt, CStateManager& mgr) {
  for (int i = 0; i < m_fields.size(); ++i) {
    m_fields[i].Think(dt, mgr, i);
  }
}

CScriptCannonBall::TrackedShot::TrackedShot(TUniqueId id, bool b)
: m_scriptObject(id), m_f(1.0), m_updateFrameIdx(0), m_b(b) {}

void CScriptCannonBall::TrackedShot::Think(float dt, CStateManager& mgr, int index) {
  if (!m_flag2) {
    return;
  }
  CScriptEffect* effect = TCastToPtr< CScriptEffect >(mgr.ObjectById(m_scriptObject));
  if (!effect) {
    return;
  }

  CPlayer* player = mgr.GetPlayer(index);
  if (m_f > 0.0f) {
    CTransform4f mat = player->GetMorphBall()->GetBallToWorld();
    const CVector3f position = mat.GetTranslation();
    CTransform4f lookXf = CTransform4f::LookAt(position, position + player->GetLookDir());
    effect->SetTransform(lookXf);
  }
  if (m_b) {
    if (!effect->IsEmitting()) {
      effect->AcceptScriptMsg(mgr,
                              CScriptMsg(kInvalidUniqueId, effect->GetUniqueId(), kSM_Activate));
      player->GetPlayerState()->SetItemAmount(CPlayerState::kIT_CannonBall, 1);
#ifdef TARGET_PC
      if (PortBallDebug()) {
        std::fprintf(stderr, "[ball] cannon flag on (origin frame %d)\n", m_updateFrameIdx);
      }
#endif
    }

    CMorphBall* morph = player->GetMorphBall();

    int uVar4 = morph->GetLastWallCollisionFrame() - m_updateFrameIdx;
    uVar4 = uVar4 > 0 ? uVar4 : 0;
    int uVar3 = morph->GetLastFloorCollisionFrame() - m_updateFrameIdx;
    uVar3 = uVar3 > 0 ? uVar3 : 0;
    if (uVar3 < uVar4) {
      uVar3 = uVar4;
    }
#ifdef TARGET_PC
    // Port: the decompiled ternary was inverted. The same "is the player in the ball" idiom
    // elsewhere in the tree reads
    //   (spawned == kMS_Unmorphed ? GetMorphballTransitionState() : kMS_Unmorphed) == kMS_Morphed
    // and this one tested the spawn state for kMS_Morphed and returned kMS_Unmorphed otherwise,
    // so a player who did not spawn in the ball - everyone who rolls into a cannon - read as
    // "not morphed" and the flag was dropped on the first tick after it was raised. The effect's
    // damage pass then never ran and the ball bounced off the wall instead of breaking it.
    const bool notMorphed = !((player->GetSpawnedMorphballState() == CPlayer::kMS_Unmorphed
                                   ? player->GetMorphballTransitionState()
                                   : CPlayer::kMS_Unmorphed) == CPlayer::kMS_Morphed);
    if (uVar3 > 5 || morph->GetBallState() == CMorphBall::kBS_Spider || notMorphed) {
#else
    if (uVar3 > 5 || morph->GetBallState() == CMorphBall::kBS_Spider ||
        (player->GetSpawnedMorphballState() == CPlayer::kMS_Morphed
             ? player->GetMorphballTransitionState()
             : CPlayer::kMS_Unmorphed) != CPlayer::kMS_Morphed) {
#endif

#ifdef TARGET_PC
      if (PortBallDebug()) {
        std::fprintf(stderr,
                     "[ball] cannon flag off: frames since hit %d, wall %d floor %d, ball state "
                     "%d, morph %d\n",
                     uVar3, morph->GetLastWallCollisionFrame(), morph->GetLastFloorCollisionFrame(),
                     int(morph->GetBallState()), int(player->GetMorphballTransitionState()));
      }
#endif
      effect->AcceptScriptMsg(mgr,
                              CScriptMsg(kInvalidUniqueId, effect->GetUniqueId(), kSM_Deactivate));
      player->GetPlayerState()->SetItemAmount(CPlayerState::kIT_CannonBall, 0);
      m_b = false;
    }

  } else {
    m_f -= dt / 0.25f;
    if (m_f < 0.0f) {
      m_f = 0.0f;
      effect->AcceptScriptMsg(mgr,
                              CScriptMsg(kInvalidUniqueId, effect->GetUniqueId(), kSM_Deactivate));
      m_flag2 = false;
    }
  }

  CColor color = CColor::White().WithAlphaOf(m_f * player->fn_8000BE98());
  effect->SetModelFlags(CModelFlags(CModelFlags::kT_One, color));
}

// @ fn_57_27C
void CScriptCannonBall::TrackedShot::OnIncrementMsg(CStateManager& mgr, bool param) {
  const bool value = param;
  m_b = value;
  m_flag2 = value;
  if (value) {
    m_updateFrameIdx = mgr.GetUpdateFrameIdx();
    m_f = 1.0f;
  }
}

// @ fn_57_24C
void CScriptCannonBall::TrackedShot::FreeScriptObject(CStateManager& mgr) {
  mgr.DeleteObjectRequest(m_scriptObject);
}

CEntity* LoadCannonBall(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrCannonBall sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrCannonBall.inc"

  return new CScriptCannonBall(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                               LdrToEntityInfo(info, sldrThis.editorProperties),
                               LdrToTransform4f(sldrThis.editorProperties), sldrThis.effect

  );
}

#ifndef MONOLITHIC
SCannonBall_FuncPtrs REL_loader_CannonBall;

// @ fn_57_70
void SetRelLoaderFunctionToLoader() {
  REL_loader_CannonBall.mLoadCannonBall = LoadCannonBall;
  SetSCannonBall_FuncPtrs(&REL_loader_CannonBall);
}

extern "C" void RELMain() { SetRelLoaderFunctionToLoader(); }

extern "C" void RELExit() { SetSCannonBall_FuncPtrs(nullptr); }
#endif
