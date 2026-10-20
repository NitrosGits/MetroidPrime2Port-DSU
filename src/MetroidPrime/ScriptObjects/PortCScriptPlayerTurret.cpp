// Host body for the ScriptPlayerTurret REL (module 63), milestone 1: the turret loads, registers,
// takes kSM_Create and thinks. The retail module is the shooting gallery's player turret: a
// script object with no model of its own that owns the turret base actor (the one with the
// model) and the turret top actor, and that the player enters to aim.
//
// Which methods the REL overrides is not guessed: its vtable lbl_63_data_40 lists them by
// address and the slots line up with CActor's declaration order (slot 5 Think, slot 6
// AcceptScriptMsg, slot 9 PreRender, slot 18 GetTouchBounds; the entries in between are the
// DOL's own CActor/CEntity symbols). AcceptScriptMsg (fn_63_1BE8, 0x784 bytes) has its message
// switch ported: the player enters the turret (kSM_Activate), leaves it (kSM_Deactivate) and the
// turret starts (kSM_Start); the four cases that are still stand-ins are named in its comment.
// Think (fn_63_23C0, 0x968 bytes) and PreRender (fn_63_2D68, 0x2CC) are too big for this slice
// and forward to CActor behind a once-only stand-in log; the five member functions the module
// hands the DOL through SPlayerTurret_FuncPtrs are ported, so the turret camera, the exit path
// and the turret HUD keep working. The analysis is docs/module-analysis/ScriptPlayerTurret.md.
#include "MetroidPrime/ScriptObjects/CScriptPlayerTurret.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/CResFactory.hpp"
#include "Kyoto/Input/CFinalInput.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAnimPlaybackParms.hpp"
#include "MetroidPrime/CControlMapper.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CGameState.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrPlayerTurret.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"

#include <math.h>
#include <stdio.h>

namespace {

// Reports a stand-in the first time it runs, so a log lists what is still a shell.
void StandIn(const char* what) {
  static const char* sSeen[16] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == what) {
      return;
    }
  }
  if (sCount < 16) {
    sSeen[sCount++] = what;
  }
  fprintf(stderr, "[turret] stand-in %s (see PortCScriptPlayerTurret.cpp)\n", what);
}

// The guest turns -1 into the invalid sfx id before storing it (fn_63_144 at 0x000004A0).
ushort TranslateSfx(int id) {
  return CSfxManager::TranslateSFXID(id == -1 ? CSfxManager::kInternalInvalidSfxId
                                             : static_cast< ushort >(id));
}

// The difference of two analog inputs is clamped to -1..1: the magnitude is compared with 1.0
// (lbl_63_rodata_10) and the value keeps its sign (lbl_63_rodata_1C = -1). The turret's input
// path uses it at 0x00001310 and 0x0000137C.
float ClampAxis(float value) {
  if (fabs(value) > 1.f) {
    return value >= 0.f ? 1.f : -1.f;
  }
  return value;
}

} // namespace

// fn_63_30E8 (0x3B4). The base call is the retail one: inGrave 0 (`li r7, 0x0` at 0x00003210),
// an empty CModelData (the turret has no model of its own; fn_63_374C at 0x00003268), the
// material list lbl_63_data_0 = 0x15 through the DOL's __shl2i helper (0x000031C8) and
// kInvalidUniqueId as the next draw node (0x000031A4).
//
// The turret's own fields are then written one by one: the one-unit box around the actor's
// position (0x00003240), the flags and the eight angles and speeds (0x0000328C..0x00003308), the
// editor transform's forward vector and translation (0x0000332C, 0x000033FC), the light colour
// (0x00003348), the two weapon effects (0x0000337C), the empty token (fn_63_349C at
// 0x00003384), the weapon damage (0x00003398..0x000033EC), the sound ids (0x00003418) and the
// flag byte at 0x22C with its two top bits cleared (0x0000342C).
CScriptPlayerTurret::CScriptPlayerTurret(TUniqueId uid, const rstl::string& name,
                                         const CEntityInfo& info, const CTransform4f& xf,
                                         const CActorParameters& actParms, uint flags,
                                         float maxHorizRotationLeft, float maxHorizRotationRight,
                                         float maxVertElevationUp, float maxVertElevationDown,
                                         float damageAngle, float horizSpeed, float vertSpeed,
                                         float fireRate, const CDamageInfo& weaponDamage,
                                         CAssetId weaponEffect, CAssetId weaponEffectMultiPlayer,
                                         ushort sfxTurretRotation, ushort sfxSinglePlayerImpact,
                                         ushort sfxMultiPlayerImpact,
                                         ushort sfxSinglePlayerProjectile,
                                         ushort sfxMultiPlayerProjectile)
: CActor(uid, name, info, 0, xf, CModelData(), CMaterialList(kMT_CameraPassthrough), actParms,
         kInvalidUniqueId)
, mBounds(CAABox::MakeMaxInvertedBox()) // guest 0x158, filled in below
, mFlags(flags)                        // guest 0x170
, mMaxHorizRotationLeft(maxHorizRotationLeft)   // guest 0x174
, mMaxHorizRotationRight(maxHorizRotationRight) // guest 0x178
, mMaxVertElevationUp(maxVertElevationUp)       // guest 0x17C
, mMaxVertElevationDown(maxVertElevationDown)   // guest 0x180
, mMaxAimAngle(damageAngle)                     // guest 0x184
, mPlayerId(kInvalidUniqueId)                   // guest 0x188
, mHorizSpeed(horizSpeed)                       // guest 0x18C
, mVertSpeed(vertSpeed)                         // guest 0x190
, mFireRate(fireRate)                           // guest 0x194
, mBaseActorId(kInvalidUniqueId)                // guest 0x198
, mTopActorId(kInvalidUniqueId)                 // guest 0x19A
, mAnimId1(0)                                   // guest 0x19C
, mAnimId2(0)                                   // guest 0x1A0
, mUnknown1A4(0)                                // guest 0x1A4
, mUnknown1A8(0)                                // guest 0x1A8
, mUnknown1AC(0)                                // guest 0x1AC
, mPitchTarget(0.f)                           // guest 0x1B0
, mPitch(0.f)                            // guest 0x1B4
, mUnknown1B8(0.f)                              // guest 0x1B8
, mUnknown1BC(0.f)                              // guest 0x1BC
, mUnknown1C0(0.f)                              // guest 0x1C0
, mBaseForward(xf.GetForward())                 // guest 0x1C4
, mYawTarget(0.f)                              // guest 0x1D0
// guest 0x1D4: the model flags the base actor had: the colour (1, 1, 1, 1) from the CColor
// constructor at 0x00003348, the blend mode 0 and the material set 0 and the flags 3 from the
// stack slots the constructor fills at 0x0000333C..0x00003344.
, mModelFlags(CModelFlags(CModelFlags::kT_Opaque, CColor(1.f, 1.f, 1.f, 1.f)))
, mWeaponEffect(weaponEffect)                   // guest 0x1E0
, mWeaponEffectMulti(weaponEffectMultiPlayer)   // guest 0x1E4
// guest 0x1E8: the empty token of fn_63_349C (0x00003384); the host's null pointer is the
// same thing, an object reference with no object in it.
, mToken(nullptr)
, mWeaponDamage(weaponDamage)                   // guest 0x1F0
, mUnknown20C(0.f)                              // guest 0x20C
, mTargetPosition(xf.GetTranslation())          // guest 0x210
, mSfxHandle(0)                                 // guest 0x21C
, mHullActorId(kInvalidUniqueId)                // guest 0x220
, mSfxTurretRotation(sfxTurretRotation)        // guest 0x222
, mSfxSinglePlayerImpact(sfxSinglePlayerImpact) // guest 0x224
, mSfxMultiPlayerImpact(sfxMultiPlayerImpact)   // guest 0x226
, mSfxSinglePlayerProjectile(sfxSinglePlayerProjectile) // guest 0x228
, mSfxMultiPlayerProjectile(sfxMultiPlayerProjectile)   // guest 0x22A
, mPlayerInTurret(false)                        // guest 0x22C bit 0x80
, mUnknown22C_6(false)                          // guest 0x22C bit 0x40
{
  // fn_63_30E8 at 0x00003240: the bounds are the actor's position grown by half a unit
  // (lbl_63_rodata_5C = 0.5).
  const CVector3f position = GetTranslation();
  mBounds = CAABox(position - CVector3f(0.5f, 0.5f, 0.5f),
                   position + CVector3f(0.5f, 0.5f, 0.5f));
}

// fn_63_3070 (0x78): the vtable is reset, the token at 0x1E8 is destroyed and CActor's
// destructor runs.
CScriptPlayerTurret::~CScriptPlayerTurret() {}

// vtable slot 5 = fn_63_23C0 (0x968): the turret's aiming, its sounds and the hull transform.
void CScriptPlayerTurret::Think(float dt, CStateManager& mgr) {
  StandIn("fn_63_23C0 (CScriptPlayerTurret::Think)");
  CActor::Think(dt, mgr);
}

// vtable slot 6 = fn_63_1BE8 (0x784): the turret's script messages.
//
// Retail switches on the message's four-character code (the comparison tree at 0x1BE8..0x1C9C)
// and every case ends in CActor::AcceptScriptMsg (0x2348). The cases ported here are retail's, in
// retail's order:
//
//   kSM_Create     (0x58435254, asm 0x1C9C): the hull's model token.
//   kSM_Activate   (0x41435456, asm 0x1DA4): the player enters the turret.
//   kSM_Deactivate (0x44435456, asm 0x1F58): the player leaves it.
//   kSM_Start      (0x53545254, asm 0x1FB0): the turret starts; its top actor becomes solid.
//   kSM_Stop       (0x53544F50, asm 0x2008): the player is ejected and killed.
//   kSM_AreaLoaded (0x58414C44, asm 0x2244): the hull's actors (only the two connections of
//                                               fn_63_15B0 are ported).
//   kSM_Damage     (0x58444D47, asm 0x224C): the damage flash and the impact sound.
void CScriptPlayerTurret::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  switch (msg.GetMessage()) {
  case kSM_Create: {
    // 0x1CA0: the multi-player weapon effect (0x1E4) in multiplayer, the single-player one
    // (0x1E0) otherwise.
    const CAssetId effect = mgr.IsMultiplayer() ? mWeaponEffectMulti : mWeaponEffect;
    // 0x1CC0: the resource type of that asset.
    const FourCC type = gpResourceFactory->GetResourceTypeById(effect);
    // 0x1CF0: its token becomes the turret's own token (0x1E8).
    mToken = rs_new CToken(gpSimplePool->GetObj(SObjectTag(type, effect)));
    break;
  }
  case kSM_Activate: {
    // 0x1DA4: the message's originator (the guest's message + 0x2) is the player that entered
    // the trigger; the turret remembers it as its player.
    mPlayerId = msg.GetOriginator();
    CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mPlayerId));
    // 0x1DC8: the player's health is the float at + 4 of its CHealthInfo (the guest's CHealthInfo
    // starts with the initial health), and a dead player cannot enter the turret.
    if (player != nullptr && player->GetHealthInfo()->GetHP() > 0.f) {
      // 0x1DF0: the turret's own unique id (the guest's + 0x8) is what the player enters.
      player->StartTurret(GetUniqueId(), mgr);
      // 0x1E0C: the flag byte's bit 0x80 - the player is in the turret.
      mPlayerInTurret = true;
    }
    // 0x1E30: both rotations start at zero.
    mPitch = 0.f;
    mPitchTarget = 0.f;
    // 0x1E38: the base actor (0x198) and the top actor (0x19A).
    CActor* base = TCastToPtr< CActor >(mgr.ObjectById(mBaseActorId));
    CActor* top = TCastToPtr< CActor >(mgr.ObjectById(mTopActorId));
    // 0x1E70: the base actor's model is read without a test in the guest - a port limit, the host
    // allows an actor without a model.
    CModelData* model = base != nullptr ? base->ModelData() : nullptr;
    // 0x1E64: the base actor needs its animation data, and the top actor has to exist.
    if (base != nullptr && model != nullptr && model->GetAnimationData() != nullptr &&
        top != nullptr) {
      // 0x1E88: the base actor keeps its own position but takes the turret's rotation.
      const CVector3f position = base->GetTranslation();
      base->SetTransform(GetTransform());
      base->SetTranslation(position);
      // 0x1EC4: the two additive animations (0x19C and 0x1A0) at weight zero.
      base->AnimationData()->AddAdditiveAnimation(mAnimId1, 0.f, false, false);
      base->AnimationData()->AddAdditiveAnimation(mAnimId2, 0.f, false, false);
      // 0x1F00: the top actor goes onto the base actor's "upwardballsnapp_LCTR" locator.
      const CTransform4f locator = model->GetLocatorTransform("upwardballsnapp_LCTR");
      top->SetTransform(base->GetTransform() * locator);
    }
    // 0x1F48: the damage flash timer.
    mUnknown20C = 0.f;
    break;
  }
  case kSM_Deactivate: {
    // 0x1F58: the player that was in the turret leaves it.
    CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mPlayerId));
    if (player != nullptr) {
      // 0x1F80: the state is 0 (kTS_None).
      player->SetTurretState(CPlayer::kTS_None, mgr);
    }
    // 0x1F94: the turret has no player any more.
    mPlayerId = kInvalidUniqueId;
    // 0x1F9C: the vtable's slot 7 with 0 - the turret is deactivated.
    SetActive(false);
    break;
  }
  case kSM_Start: {
    // 0x1FB0: only an active turret starts (the guest's byte 0x20 is CEntity's active flag).
    if (!GetActive()) {
      break;
    }
    CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mPlayerId));
    if (player != nullptr) {
      // 0x1FE0: the state is 3 (kTS_Active).
      player->SetTurretState(CPlayer::kTS_Active, mgr);
    }
    // 0x1FF0: fn_63_14B4 - the top actor becomes solid.
    AddSolidToTopActor(mgr);
    // 0x1FFC: the material is 0x29 (kMT_Orbit): the turret orbits the player.
    AddMaterial(kMT_Orbit, mgr);
    break;
  }
  case kSM_Stop: {
    // 0x2008: only an active turret stops (the guest's byte 0x20 is CEntity's active flag).
    if (!GetActive()) {
      break;
    }
    // 0x201C: the turret loses the orbit material (0x29).
    RemoveMaterial(kMT_Orbit, mgr);
    // 0x2020: the base actor (0x198) gets the turret's saved model flags back (the guest's
    // 0x1D4..0x1DC).
    CActor* base = TCastToPtr< CActor >(mgr.ObjectById(mBaseActorId));
    if (base != nullptr) {
      // 0x2040
      base->SetModelFlags(mModelFlags);
    }
    // 0x2068: the player that was in the turret.
    CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mPlayerId));
    if (player == nullptr) {
      // 0x2228: no player, so the turret is only deactivated.
      SetActive(false);
      break;
    }
    // 0x209C: the player is ejected from the turret.
    player->EjectFromTurret(GetUniqueId(), mgr);
    // 0x20C4: and killed by the hull: the hull actor id is 0x220.
    mgr.KillPlayer(player->GetHealthInfo()->GetHP(), mPlayerId, mHullActorId);
    // 0x20C8: the player is no longer in the turret.
    mPlayerInTurret = false;
    // 0x20EC: and the turret has no player any more.
    mPlayerId = kInvalidUniqueId;
    // 0x20F0: the base actor and the top actor (0x19A) again.
    base = TCastToPtr< CActor >(mgr.ObjectById(mBaseActorId));
    CActor* top = TCastToPtr< CActor >(mgr.ObjectById(mTopActorId));
    if (base != nullptr) {
      // 0x2124: both rotations start at zero.
      mPitch = 0.f;
      mPitchTarget = 0.f;
      if (top != nullptr) {
        // 0x213C: the base actor keeps its own position but takes the turret's rotation.
        const CVector3f position = base->GetTranslation();
        base->SetTransform(GetTransform());
        base->SetTranslation(position);
        // 0x216C: the base actor's animation data.
        CAnimData* animData = base->AnimationData();
        // 0x2178: the two additive animations (0x19C and 0x1A0) at weight zero.
        animData->AddAdditiveAnimation(mAnimId1, 0.f, false, false);
        animData->AddAdditiveAnimation(mAnimId2, 0.f, false, false);
        // 0x21D4: the animation data's flag byte at 0x2AC loses bit 0x40 (the loop flag) and
        // gains bit 0x80 (the animating flag) - the host's EnableLooping does both.
        animData->EnableLooping(false);
        // 0x21F0: the turret's own animation (0x1AC) plays: the id, -1, the weight 1.0 and
        // "animating".
        animData->SetAnimation(CAnimPlaybackParms(mUnknown1AC, -1, 1.f, true), true);
      }
    }
    // 0x2228: the turret is deactivated.
    SetActive(false);
    break;
  }
  case kSM_AreaLoaded: {
    // 0x2244: fn_63_15B0 (0x3C4 bytes) - the hull's base and top actors and its five animation
    // ids. Only its two connections are ported (see LoadHullActors).
    LoadHullActors(mgr);
    break;
  }
  case kSM_Damage: {
    // 0x2254: the damage flash timer (lbl_63_rodata_20 = 0.15).
    mUnknown20C = 0.15f;
    if (mgr.IsMultiplayer()) {
      // 0x2268: the player in the turret.
      CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mPlayerId));
      if (player != nullptr) {
        // 0x22BC: the multi-player impact sound (0x226) at the player's sound pan.
        CSfxManager::SfxStart(mSfxMultiPlayerImpact, 0x7f,
                              player->GetSoundPan(CPlayer::kMSP_Player), CSfxManager::kAllAreas,
                              false, false, CSfxManager::kMedPriority);
      }
    } else {
      // 0x22E4: the single-player impact sound (0x224) as an emitter at the turret's target
      // position (0x210), in the turret's own area.
      CSfxManager::AddEmitter(mSfxSinglePlayerImpact, mTargetPosition, GetCurrentAreaId().Value(),
                              true, false, CSfxManager::kMedPriority);
    }
    // 0x22E8: the damaged actor - the message's sender - gives the hull actor id.
    const CActor* damaged = TCastToConstPtr< CActor >(mgr.GetObjectById(msg.GetSenderId()));
    if (damaged != nullptr) {
      const CHealthInfo* health = damaged->GetHealthInfo();
      if (health != nullptr) {
        // 0x2338: the owner of the last damage (the guest's CHealthInfo + 0x18).
        mHullActorId = health->GetLastDamageOwner();
      } else {
        // 0x2344: the message's originator.
        mHullActorId = msg.GetOriginator();
      }
    }
    break;
  }
  default:
    break;
  }
  // 0x2348: every case ends in CActor's own handler.
  CActor::AcceptScriptMsg(mgr, msg);
}

// fn_63_14B4 (0x54): the turret's top actor (0x19A) becomes solid.
void CScriptPlayerTurret::AddSolidToTopActor(CStateManager& mgr) {
  // 0x14CC: the top actor's id.
  CActor* top = TCastToPtr< CActor >(mgr.ObjectById(mTopActorId));
  // 0x14E0
  if (top != nullptr) {
    // 0x14EC: the material is 0x3b (kMT_Solid).
    top->AddMaterial(kMT_Solid, mgr);
  }
}

// fn_63_15B0 (0x3C4): the base actor (0x198) and the top actor (0x19A) come from the turret's
// "AttachedAnimatedObject" and "AttachedCollisionObject" connections, and the base actor's
// animation database gives the five additive animation ids (0x19C..0x1AC) and the light colour
// (0x1D4..0x1DC).
//
// Only the two connections are ported here: the animation lookup is 0x2B4 bytes of it and stays a
// stand-in. The connections have to be real, because the DOL's turret HUD (CTurretHud::
// UpdateEnergy) reads the hull actor's id and dereferences the actor it names.
void CScriptPlayerTurret::LoadHullActors(CStateManager& mgr) {
  // 0x15E4: the base actor is the turret's "AttachedAnimatedObject" connection.
  mBaseActorId = FindConnectedObject(mgr, kSS_AttachedAnimatedObject, kSM_Attach);
  // 0x1608: the base actor itself.
  CActor* base = TCastToPtr< CActor >(mgr.ObjectById(mBaseActorId));
  // 0x161C: the base actor's animation data; the guest reads the model pointer without a test
  // (a port limit: the host allows an actor without a model).
  if (base != nullptr && base->GetModelData() != nullptr &&
      base->GetModelData()->GetAnimationData() != nullptr) {
    // 0x1918: the base actor's model flags (the guest's 0xFC..0x104) become the turret's saved
    // copy (0x1D4..0x1DC), which the stop message hands back.
    mModelFlags = base->GetModelFlags();
  }
  // The base actor's animation database gives the five animation ids (0x19C..0x1AC); that part of
  // fn_63_15B0 is still a stand-in.
  StandIn("fn_63_15B0 (the hull's animation ids)");
  // 0x1930: the top actor is the turret's "AttachedCollisionObject" connection.
  mTopActorId = FindConnectedObject(mgr, kSS_AttachedCollisionObject, kSM_Attach);
}

// vtable slot 9 = fn_63_2D68 (0x2CC): the turret's light colour on the hull.
void CScriptPlayerTurret::PreRender(CStateManager& mgr) {
  StandIn("fn_63_2D68 (CScriptPlayerTurret::PreRender)");
  CActor::PreRender(mgr);
}

// vtable slot 18 = fn_63_3034 (0x3C): the touch bounds are the turret's own box.
rstl::optional_object< CAABox > CScriptPlayerTurret::GetTouchBounds() const {
  return rstl::optional_object< CAABox >(mBounds);
}

// fn_63_1070 (0xC4). The camera looks from the turret's position towards the turret's position
// plus its forward vector flattened onto the ground plane (the z term is rodata_8 = 0.0).
CTransform4f CScriptPlayerTurret::GetCameraTransform(CStateManager& mgr) {
  const CTransform4f turret = GetTurretTransform(mgr);
  const CVector3f position = turret.GetTranslation();
  const CVector2f forward(turret.GetForward().GetX(), turret.GetForward().GetY());
  return CTransform4f::LookAt(position,
                              CVector3f(position.GetX() + forward.GetX(),
                                        position.GetY() + forward.GetY(), position.GetZ() + 0.f),
                              CVector3f::Up());
}

// fn_63_F6C (0xB4). The turret sits on the base actor's "upwardballsnapp_LCTR" locator
// (lbl_63_rodata_60 + 7); without a base actor the turret's own transform is used.
CTransform4f CScriptPlayerTurret::GetTurretTransform(CStateManager& mgr) {
  const CActor* base = TCastToConstPtr< CActor >(mgr.ObjectById(mBaseActorId));
  if (base == nullptr) {
    return GetTransform();
  }
  const CModelData* model = base->GetModelData();
  if (model == nullptr) {
    // Port limit: the guest reads the base actor's model without a test (0x00000FD0); on the
    // host the base actor can be a script actor without a model.
    return GetTransform();
  }
  const CTransform4f locator = model->GetLocatorTransform("upwardballsnapp_LCTR");
  return base->GetTransform() * locator;
}

// fn_63_1B70 (0x78). The player is let out of the turret and the hull loses its solid material.
void CScriptPlayerTurret::ExitTurret(CStateManager& mgr) {
  mPlayerInTurret = false; // guest 0x22C bit 0x80, cleared at 0x00001BA0
  SendScriptMsgs(kSS_Exited, mgr, kInvalidUniqueId, kSM_None);
  // 0x1BCC: fn_63_1460 - the top actor (0x19A) loses kMT_Solid.
  RemoveSolidFromTopActor(mgr);
}

// fn_63_1460 (0x54): the turret's top actor (0x19A) loses kMT_Solid.
void CScriptPlayerTurret::RemoveSolidFromTopActor(CStateManager& mgr) {
  // 0x1478: the top actor's id.
  CActor* top = TCastToPtr< CActor >(mgr.ObjectById(mTopActorId));
  // 0x148C
  if (top != nullptr) {
    // 0x1498: the material is 0x3b (kMT_Solid).
    top->RemoveMaterial(kMT_Solid, mgr);
  }
}

// fn_63_1134 (0x32C): the player's aim while in the turret.
void CScriptPlayerTurret::ProcessInput(const CFinalInput& input, CStateManager& mgr) {
  // 0x1154: the player in the turret (0x188) and the base actor (0x198).
  CPlayer* player = TCastToPtr< CPlayer >(mgr.ObjectById(mPlayerId));
  CActor* base = TCastToPtr< CActor >(mgr.ObjectById(mBaseActorId));
  if (player == nullptr || base == nullptr) {
    return;
  }
  // 0x1198: the player's turret state (the guest's player + 0x12F8) has to be 3 (kTS_Active).
  const CControlMapper& mapper = player->GetControlMapper();
  if (player->GetTurretState() == CPlayer::kTS_Active) {
    // 0x11AC: the reload timer (0x1B8) counts down.
    if (mUnknown1B8 > 0.f) {
      // 0x11BC: the guest's input + 0 is the frame time.
      mUnknown1B8 -= input.DeltaTime();
      // 0x11C4: the base actor's animation data; the guest reads the model pointer without a
      // test (a port limit: the host allows an actor without a model).
      CAnimData* animData =
          base->GetModelData() != nullptr ? base->AnimationData() : nullptr;
      if (animData != nullptr) {
        // 0x11D4: the time left of the "Whole Body" animation (lbl_63_rodata_60 + 0x1B).
        if (animData->GetAnimTimeRemaining("Whole Body") <= 0.f) {
          // 0x1220: the animation data's flag byte at 0x2AC gains bit 0x40 (the loop flag) and
          // bit 0x80 (the animating flag) - the host's EnableLooping does both.
          animData->EnableLooping(true);
          // 0x125C: the turret's animation (0x1A8) plays: the id, -1, the weight 1.0 and
          // "animating".
          animData->SetAnimation(CAnimPlaybackParms(mUnknown1A8, -1, 1.f, true), false);
        }
      }
    }
    // 0x1294: the beam is fired.
    if (player->FireBeamPressed(input)) {
      // 0x12A8: only when the reload timer is over.
      if (mUnknown1B8 == 0.f) {
        // 0x12C0: fn_63_84C fires the turret's shot.
        FireTurretShot(mgr);
        // 0x12CC: the reload timer is the fire rate (0x194).
        mUnknown1B8 = mFireRate;
      }
    }
    // 0x12D4: the turn axis: the guest's commands 4 and 3 (turn right and turn left).
    float yaw = mapper.GetAnalogInput(CControlMapper::kC_TurnLeft, input,
                                      CControlMapper::kFT_Filtered) -
                mapper.GetAnalogInput(CControlMapper::kC_TurnRight, input,
                                      CControlMapper::kFT_Filtered);
    // 0x1300: the difference is clamped to -1..1.
    yaw = ClampAxis(yaw);
    // 0x1334: the yaw the input asks for.
    mYawTarget = yaw;
  }
  // 0x1338: the pitch axis: the guest's commands 1 and 2 (forward and backward).
  float pitch = mapper.GetAnalogInput(CControlMapper::kC_Backward, input,
                                      CControlMapper::kFT_Filtered) -
                mapper.GetAnalogInput(CControlMapper::kC_Forward, input,
                                      CControlMapper::kFT_Filtered);
  // 0x1364: the difference is clamped to -1..1.
  pitch = ClampAxis(pitch);
  // 0x1398: with the Y axis inverted (the guest's game state byte 0xA4, bit 0x40) the two
  // commands swap.
  if (gpGameState->GameOptions().GetInvertYAxis()) {
    // 0x13B4
    pitch = mapper.GetAnalogInput(CControlMapper::kC_Forward, input,
                                  CControlMapper::kFT_Filtered) -
            mapper.GetAnalogInput(CControlMapper::kC_Backward, input,
                                  CControlMapper::kFT_Filtered);
    // 0x13F0
    pitch = ClampAxis(pitch);
  }
  // 0x1418: the pitch is scaled by the elevation limits: down for a negative input, up for a
  // positive one.
  if (pitch < 0.f) {
    // 0x1428
    mPitchTarget = pitch * mMaxVertElevationUp;
  } else {
    // 0x1438
    mPitchTarget = pitch * mMaxVertElevationDown;
  }
}

// fn_63_84C (0x184): the turret's shot. Too big for this slice: it stays a stand-in that logs
// once.
void CScriptPlayerTurret::FireTurretShot(CStateManager& mgr) {
  StandIn("fn_63_84C (the turret's shot)");
}

// fn_63_840 (0xC): the actor the turret HUD reads the hull's health from.
TUniqueId CScriptPlayerTurret::GetHullActorId() {
  return mTopActorId;
}

// fn_63_144 (0x4CC). The property loop is the generated one (SLdrPlayerTurret.inc); the loader
// turns the seven angles and speeds from degrees into radians (lbl_63_rodata_0 = 0.017453292)
// and leaves the fire rate alone.
static CEntity* PortLoadScriptPlayerTurret(CStateManager& mgr, CInputStream& input,
                                            CEntityInfo& info) {
  SLdrPlayerTurret sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrPlayerTurret.inc"

  const float kDegToRad = 0.017453292f;

  return rs_new CScriptPlayerTurret(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties),
      LdrToTransform4f(sldrThis.editorProperties), CActorParameters(), sldrThis.flagsPlayerTurret,
      kDegToRad * sldrThis.maxHorizRotationLeft, kDegToRad * sldrThis.maxHorizRotationRight,
      kDegToRad * sldrThis.maxVertElevationUp, kDegToRad * sldrThis.maxVertElevationDown,
      kDegToRad * sldrThis.damageAngle, kDegToRad * sldrThis.horizSpeed,
      kDegToRad * sldrThis.vertSpeed, sldrThis.fireRate,
      LdrToDamageInfo(sldrThis.weaponDamage), sldrThis.weaponEffect,
      sldrThis.weaponEffectMultiPlayer, TranslateSfx(sldrThis.sFXTurretRotation),
      TranslateSfx(sldrThis.sFXSinglePlayerImpact), TranslateSfx(sldrThis.sFXMultiPlayerImpact),
      TranslateSfx(sldrThis.sFXSinglePlayerProjectile),
      TranslateSfx(sldrThis.sFXMultiPlayerProjectile));
}

static SPlayerTurret_FuncPtrs REL_loader_PlayerTurret;

// Retail's RELMain calls fn_63_70, which stores the loader fn_63_144 at lbl_63_bss_20 +0x0 and
// copies the five pointers-to-member lbl_63_data_4 = {0, -1, fn_63_1070},
// lbl_63_data_10 = {0, -1, fn_63_F6C}, lbl_63_data_1C = {0, -1, fn_63_1B70},
// lbl_63_data_28 = {0, -1, fn_63_1134} and lbl_63_data_34 = {0, -1, fn_63_840} to +0x4 .. +0x3C,
// then calls the DOL's SetSPlayerTurret_FuncPtrs. The DOL's proxies in ScriptLoaderRel.cpp call
// through it, so the turret camera, the exit path, the input path and the turret HUD work.
void RELMain() {
  REL_loader_PlayerTurret.mLoadPlayerTurret = PortLoadScriptPlayerTurret;
  REL_loader_PlayerTurret.mGetCameraTransform =
      static_cast< CTransform4f (CEntity::*)(CStateManager&) >(
          &CScriptPlayerTurret::GetCameraTransform);
  REL_loader_PlayerTurret.mGetTurretTransform =
      static_cast< CTransform4f (CEntity::*)(CStateManager&) >(
          &CScriptPlayerTurret::GetTurretTransform);
  REL_loader_PlayerTurret.mExitTurret =
      static_cast< void (CEntity::*)(CStateManager&) >(&CScriptPlayerTurret::ExitTurret);
  REL_loader_PlayerTurret.mProcessInput =
      static_cast< void (CEntity::*)(const CFinalInput&, CStateManager&) >(
          &CScriptPlayerTurret::ProcessInput);
  REL_loader_PlayerTurret.mGetHullActorId =
      static_cast< TUniqueId (CEntity::*)() >(&CScriptPlayerTurret::GetHullActorId);
  SetSPlayerTurret_FuncPtrs(&REL_loader_PlayerTurret);
}

void RELExit() { SetSPlayerTurret_FuncPtrs(nullptr); }
