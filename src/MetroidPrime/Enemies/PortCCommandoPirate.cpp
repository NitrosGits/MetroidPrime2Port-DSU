// Host body for the CommandoPirate REL (module 9), milestone 1: the actor loads, stands and
// thinks. Every vtable override that belongs to retail's AI forwards to CPatterned behind a
// once-only StandIn log, and the REL function it stands in for is named on each. The state
// machine (the module's trigger, state and code tables), the melee, the E-grenade, the shield and
// the burst fire are milestone 2 and forward - docs/module-analysis/CommandoPirate.md has the plan.
//
// Which methods the REL overrides is not guessed: its vtable lbl_9_data_828 (82 words, two leading
// zeros) lists them by address and the slots line up with CPatterned's declaration order (the
// entries the REL inherits are spelled out by name in the same table). The tiny accessors at the
// head of the module's .text (fn_9_0, fn_9_8, fn_9_10, fn_9_70, fn_9_78, fn_9_80, fn_9_A4) have
// four-to-twelve-instruction bodies, so they are real rather than stand-ins; fn_9_C2CC (the
// damage vulnerability) and fn_9_D474 (the projectile info) are real too.
#include "MetroidPrime/Enemies/CCommandoPirate.hpp"

#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrCommandoPirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

#include <stdio.h>
#include "rstl/string.hpp"

namespace {

// Reports a stand-in the first time it runs, so a log lists what is still a shell.
void StandIn(const char* what) {
  static const char* sSeen[32] = {};
  static int sCount = 0;
  for (int i = 0; i < sCount; ++i) {
    if (sSeen[i] == what) {
      return;
    }
  }
  if (sCount < 32) {
    sSeen[sCount++] = what;
  }
  fprintf(stderr, "[cmdopirate] %s is a port stand-in (see PortCCommandoPirate.cpp)\n", what);
}

} // namespace

// fn_9_D904. The base call is the retail one: EPatternedAI 5 (`li r4, 0x5` at 0x0000DA54), flavor
// 0 (`li r7, 0x0` at 0x0000DA40), movement 0 / collider 1 / body 1 (the words stored to the stack
// arguments at 0x0000DA50..0x0000DA5C). The path search is fn_9_D904's own call at 0x0000D9B4:
// CPFArea null, flags 1, the patterned info's pathfinding index (+0xEC), radii 1.0f (rodata_43C)
// and no avoidance filter, root position 0 (kRP_Center).
CCommandoPirate::CCommandoPirate(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                                 const CTransform4f& xf, const CModelData& modelData,
                                 const CActorParameters& actParms, const CPatternedInfo& pInfo,
                                 const CCommandoPirateData& data)
: CPatterned(static_cast< EPatternedAI >(0x5), uid, name, kFT_Zero, info, xf, modelData, pInfo,
             kMT_Ground, kCT_One, kBT_BiPedal, actParms)
, mData(data)                                     // guest 0x7c0, copied by fn_9_EA24 @0x0000D980
, mGuest920(-1)                                   // guest 0x920, fn_9_D904 @0x0000D994
, mGuest924(-1)                                   // guest 0x924, fn_9_D904 @0x0000D9A4
, mPathFindSearch(nullptr, 1, pInfo.GetPathfindingIndex(), 1.f, 1.f, 0,
                  CPFRegion::kRP_Center)           // guest 0x928, fn_9_D904 @0x0000D990
, mProjectileInfo(data.mProjectile, data.mProjectileDamage) // guest 0xa14, fn_9_D904 @0x0000D9B8
, mGuestA3C(0)                                    // guest 0xa3c, fn_9_D904 @0x0000D9D0
, mGuestA40(0)                                    // guest 0xa40, fn_9_D904 @0x0000D9D8
, mShieldVulnerability(data.mShield.mChargeVulnerability) // guest 0xa44, fn_9_D904 @0x0000D9D4
, mBoneTracking(*AnimationData(), rstl::string_l("Head_1"), 1.2217305f, 3.1415927f,
                kBTF_None)                        // guest 0xa74, fn_9_D904 @0x0000DA08
, mLocatorHead(CSegId::Invalid())                 // guest 0xc5c, fn_9_D904 @0x0000E29C
, mLocatorMidLaunch(CSegId::Invalid())            // guest 0xc5d, fn_9_D904 @0x0000E2D0
, mLocatorGun(CSegId::Invalid())                  // guest 0xc5e, fn_9_D904 @0x0000E304
, mLocatorGun2(CSegId::Invalid())                 // guest 0xc5f, fn_9_D904 @0x0000E338
, mLocatorRWrist(CSegId::Invalid())               // guest 0xc60, fn_9_D904 @0x0000E36C
, mLocatorRElbow(CSegId::Invalid())               // guest 0xc61, fn_9_D904 @0x0000E3A0
, mLocatorLWrist(CSegId::Invalid())               // guest 0xc62, fn_9_D904 @0x0000E3D4
, mGuestC63(0)                                    // guest 0xc63, fn_9_D904 @0x0000E0F0
, mGuestC64(0)                                    // guest 0xc64, fn_9_D904 @0x0000E150
, mGuestC65(0x80)                                 // guest 0xc65, fn_9_D904 @0x0000E1B0
, mGuestC66(0)                                    // guest 0xc66, fn_9_D904 @0x0000E1E8
{
  // The retail constructor sets knock-back physics on CPatterned's own manager (@0x0000E264) to
  // !((byte at +0x91c >> 2) & 1) - that is, to the manager's own default while the flag bit is
  // clear - and sets the base's flag byte at +0x681. The rest - the CVisualKnowledge, the six
  // locked tokens, the two CElementGen pointers, the burst fire and the three animation distances
  // - belongs to the state machine and the render paths this milestone does not enter; the
  // analysis note lists them.
  KnockBackController().EnableKnockBackPhysics(true);

  // The seven locator segment ids (fn_9_D904 @0x0000E26C..0x0000E3F0) are read from the model's
  // animation data. The names are the module's own rodata strings, at "StateOver" + 0x3cd and
  // friends: "Head_1", "Mid_Launch_LCTRL", "gun_LCTR" (twice), "R_wrist", "R_elbow", "L_wrist".
  if (const CAnimData* animData = GetAnimationData()) {
    mLocatorHead = animData->GetLocatorSegId(rstl::string_l("Head_1"));
    mLocatorMidLaunch = animData->GetLocatorSegId(rstl::string_l("Mid_Launch_LCTRL"));
    mLocatorGun = animData->GetLocatorSegId(rstl::string_l("gun_LCTR"));
    mLocatorGun2 = animData->GetLocatorSegId(rstl::string_l("gun_LCTR"));
    mLocatorRWrist = animData->GetLocatorSegId(rstl::string_l("R_wrist"));
    mLocatorRElbow = animData->GetLocatorSegId(rstl::string_l("R_elbow"));
    mLocatorLWrist = animData->GetLocatorSegId(rstl::string_l("L_wrist"));
  }
}

CCommandoPirate::~CCommandoPirate() {}

// A body state is only ever installed by CBodyController::Activate, and CPatterned::Think /
// ThinkAboutMove / Death / KnockBack all dereference the current state. The analysis does not say
// where retail activates it, so the port activates on kSM_Create and, as the other patterned
// actors in this port do, again on demand in every entry point that can run first.
void CCommandoPirate::EnsureBodyActive(CStateManager& mgr) {
  if (BodyController() != nullptr && !BodyController()->GetIsActive()) {
    BodyController()->Activate(mgr, pas::kAS_Invalid);
  }
}

// vtable slot 0x10 = fn_9_CAC8 (0x78).
void CCommandoPirate::PreThink(float dt, CStateManager& mgr) {
  StandIn("CCommandoPirate::PreThink (vtable slot 0x10, fn_9_CAC8)");
  EnsureBodyActive(mgr);
  CPatterned::PreThink(dt, mgr);
}

// vtable slot 0x14 = fn_9_C944 (0x184).
void CCommandoPirate::Think(float dt, CStateManager& mgr) {
  StandIn("CCommandoPirate::Think (vtable slot 0x14, fn_9_C944)");
  if (!GetActive()) {
    return;
  }
  EnsureBodyActive(mgr);
  CPatterned::Think(dt, mgr);
}

// vtable slot 0x18 = fn_9_CB40 (0x754): the message switch that activates the body and starts the
// module's state machine.
void CCommandoPirate::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  StandIn("CCommandoPirate::AcceptScriptMsg (vtable slot 0x18, fn_9_CB40)");
  CPatterned::AcceptScriptMsg(mgr, msg);
  if (msg.GetMessage() == kSM_Create) {
    EnsureBodyActive(mgr);
  }
}

// vtable slot 0x24 = fn_9_C6D4 (0xc4).
void CCommandoPirate::PreRender(CStateManager& mgr) {
  StandIn("CCommandoPirate::PreRender (vtable slot 0x24, fn_9_C6D4)");
  CPatterned::PreRender(mgr);
}

// vtable slot 0x28 = fn_9_C4FC (0x20): a straight call to CPatterned::AddToRenderer, so the base's
// is what runs.
void CCommandoPirate::AddToRenderer(const CStateManager& mgr) const {
  CPatterned::AddToRenderer(mgr);
}

// vtable slot 0x2c = fn_9_C598 (0x13c).
void CCommandoPirate::Render(const CStateManager& mgr) const {
  StandIn("CCommandoPirate::Render (vtable slot 0x2c, fn_9_C598)");
  CPatterned::Render(mgr);
}

// vtable slot 0x34 = fn_9_C51C (0x7c).
void CCommandoPirate::PreRenderAllViewports(CStateManager& mgr) {
  StandIn("CCommandoPirate::PreRenderAllViewports (vtable slot 0x34, fn_9_C51C)");
  CPatterned::PreRenderAllViewports(mgr);
}

// vtable slot 0x40 = fn_9_C2CC (0x34): the shield's damage vulnerability while the flag byte at
// guest +0xc65 has its 0x04 bit set (the guest tests it with extrwi at 0x0000C2DC), the actor's
// own vulnerability otherwise.
const CDamageVulnerability* CCommandoPirate::GetDamageVulnerability() const {
  if (mGuestC65 & 0x04) {
    return &mShieldVulnerability;
  }
  return CPatterned::GetDamageVulnerability();
}

// vtable slot 0x70 = fn_9_C798 (0x1ac): the user-event switch that starts the melee, the
// E-grenade and the shield charge.
void CCommandoPirate::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node,
                                      EUserEventType type, float dt) {
  StandIn("CCommandoPirate::DoUserAnimEvent (vtable slot 0x70, fn_9_C798)");
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}

// vtable slot 0x84 = fn_9_BF44 (0x2b0): what the blade and the shield hit.
void CCommandoPirate::CollidedWith(const TUniqueId& id, const CCollisionInfoList& list,
                                    CStateManager& mgr) {
  StandIn("CCommandoPirate::CollidedWith (vtable slot 0x84, fn_9_BF44)");
  CPatterned::CollidedWith(id, list, mgr);
}

// vtable slot 0x9c = fn_9_C300 (0x1fc).
void CCommandoPirate::KnockBack(CStateManager& mgr, const CKnockBackInfo& info) {
  StandIn("CCommandoPirate::KnockBack (vtable slot 0x9c, fn_9_C300)");
  CPatterned::KnockBack(mgr, info);
}

// vtable slot 0xa8 = fn_9_A4 (0x8): `li r3, 0x1`, so the actor can always be shot.
bool CCommandoPirate::CanBeShot(const CStateManager& mgr, int attributes) { return true; }

// vtable slot 0xac = fn_9_8 (0x8): `li r3, 0x1`, so the actor always listens.
bool CCommandoPirate::IsListening() const { return true; }

// vtable slot 0xb0 = fn_9_BE4C (0xf8): what the actor hears.
bool CCommandoPirate::Listen(CStateManager& mgr, const CVector3f& position, EListenNoiseType type) {
  StandIn("CCommandoPirate::Listen (vtable slot 0xb0, fn_9_BE4C)");
  return CPatterned::Listen(mgr, position, type);
}

// vtable slot 0xdc = fn_9_D474 (0x8): `addi r3, r3, 0xa14`, the projectile info at guest +0xa14.
CProjectileInfo* CCommandoPirate::ProjectileInfo() { return &mProjectileInfo; }

// vtable slot 0xe0 = fn_9_0 (0x8): `addi r3, r3, 0x928`, the path search at guest +0x928.
CPathFindSearch* CCommandoPirate::GetSearchPath() { return &mPathFindSearch; }

// vtable slot 0xe4 = fn_9_70 (0x8): `li r3, 0x0` - this actor has no point-based path.
CPathFindPointSearch* CCommandoPirate::GetPointSearchPath() { return nullptr; }

// vtable slot 0xe8 = fn_9_C1F4 (0xd8): the contact damage, which the module's own blade damage
// feeds in place of the patterned info's.
CDamageInfo CCommandoPirate::GetContactDamage() const {
  StandIn("CCommandoPirate::GetContactDamage (vtable slot 0xe8, fn_9_C1F4)");
  return CPatterned::GetContactDamage();
}

// vtable slot 0xf0 = fn_9_BDE0 (0x6c): the module's own trigger, state and code tables are
// installed here in retail; the base class's tables are what runs.
void CCommandoPirate::SetupStateMachine(CStateManager& mgr) {
  StandIn("CCommandoPirate::SetupStateMachine (vtable slot 0xf0, fn_9_BDE0)");
  CPatterned::SetupStateMachine(mgr);
}

// vtable slot 0xf4 = fn_9_78 (0x8): `li r3, 0x0` - this actor has no rag doll.
CRagDoll* CCommandoPirate::GetRagDoll() const { return nullptr; }

// vtable slot 0x10c = fn_9_80 (0x10): stores kInvalidUniqueId, so the actor never reports an
// attack target through this accessor.
TUniqueId CCommandoPirate::GetAttackTarget() const { return kInvalidUniqueId; }

// vtable slot 0x114 = fn_9_10 (0xc): the module's own rodata_400, the float 50.
float CCommandoPirate::GetGravityConstant() const { return 50.f; }

// ---------------------------------------------------------------------------------------------
// The module's loader (fn_9_168) and entry points.

// fn_9_168. The property ids and their order are the generated SLdrCommandoPirate reader's, which
// the REL's own switch matches id by id (fn_9_168 @0x000001C8..0x00000568).
static CEntity* PortLoadCommandoPirate(CStateManager& mgr, CInputStream& input,
                                        CEntityInfo& info) {
  SLdrCommandoPirate sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrCommandoPirate.inc"

  rstl::optional_object< CModelData > modelData(LdrToModelData(
      sldrThis.editorProperties.transform.scale, kInvalidAssetId,
      sldrThis.patterned.animationInformation, true));
  if (!modelData) {
    // The REL checks the copy's valid byte (sp+0x2d0 at 0x000006B0) and returns null when the
    // model is empty.
    return nullptr;
  }

  // The two damage infos the loader converts (fn_9_168 @0x000005E0 and @0x00000738): the blade's
  // and the projectile's.
  const CDamageInfo bladeDamage = LdrToDamageInfo(sldrThis.bladeDamage);
  const CDamageInfo projectileDamage = LdrToDamageInfo(sldrThis.projectileDamage);

  // The E-grenade half (fn_9_98C then fn_9_F50C, out of the 0xfb435257 property block).
  const SLdrUnknownStruct10& grenade = sldrThis.unknown_0xfb435257;
  const CCommandoPirateGrenadeData grenadeData(
      grenade.grenadeMass, grenade.unknown_0xed086ce0, bladeDamage, grenade.grenadeNumBounces,
      grenade.grenadeExplosion, grenade.grenadeExplosion, grenade.grenadeTrail,
      grenade.grenadeEffect, static_cast< ushort >(grenade.sound_GrenadeBounce),
      grenade.eMPDuration, grenade.grenadeAttackChance, grenade.grenadeMinAttackInterval,
      grenade.grenadePostAttackPause, grenade.grenadeMinAttackDist, grenade.grenadeMaxAttackDist,
      grenade.grenadeMinLaunchSpeed, grenade.grenadeMaxLaunchSpeed);

  // The shield half (fn_9_EF80, out of the 0x388e16c9 property block read by fn_9_1308).
  const SLdrCommandoShield& shield = sldrThis.shieldInfo;
  const CCommandoPirateShieldData shieldData(
      LdrToDamageInfo(shield.shieldChargeDamage),
      LdrToDamageVulnerability(shield.shieldVulnerability), shield.shieldChargeMinAttackDist,
      shield.shieldChargeMaxAttackDist, shield.shieldChargeSpeed, shield.shieldExplodeEffect,
      static_cast< ushort >(shield.sound_ShieldExplode), shield.unknown_0x6cb0da5a,
      shield.unknown_0xc3938663, shield.armShieldChance, shield.armShieldTime,
      shield.armShieldTimeVariation, shield.armShieldExplodeEffect, shield.shieldChargeEffect,
      shield.armShieldEffect, static_cast< ushort >(shield.sound_ShieldTurnOn),
      static_cast< ushort >(shield.sound_ShieldTurnOff));

  // The block's last two bytes (fn_9_EDC0 @0x0000EED4..0x0000EF60) hold eight and four bits of
  // the "sound" property, taken in the guest's order: 0x80 <- bit 14, 0x40 <- bit 11, 0x20 <- bit
  // 8, 0x10 <- bit 5, 0x08 <- bit 2, 0x04 <- bit 31, 0x02 <- bit 28, 0x01 <- bit 25; the second
  // byte 0x80 <- bit 6, 0x40 <- bit 3, 0x20 <- bit 0, 0x10 <- bit 29.
  const uint sound = static_cast< uint >(sldrThis.sound);
  const uchar flags0 = static_cast< uchar >((((sound >> 14) & 1) << 7) | (((sound >> 11) & 1) << 6) |
                                             (((sound >> 8) & 1) << 5) | (((sound >> 5) & 1) << 4) |
                                             (((sound >> 2) & 1) << 3) | (((sound >> 31) & 1) << 2) |
                                             (((sound >> 28) & 1) << 1) | ((sound >> 25) & 1));
  const uchar flags1 = static_cast< uchar >((((sound >> 6) & 1) << 7) | (((sound >> 3) & 1) << 6) |
                                             ((sound & 1) << 5) | (((sound >> 29) & 1) << 4));

  const CCommandoPirateData data(
      static_cast< ushort >(sldrThis.sound_Impact), static_cast< ushort >(sldrThis.sound_Hurled),
      static_cast< ushort >(sldrThis.sound_Death), static_cast< ushort >(sldrThis.alwaysFF),
      sldrThis.alwaysFF_0x467c3d94, bladeDamage, sldrThis.projectile, projectileDamage,
      static_cast< ushort >(sldrThis.sound_Projectile), sldrThis.aggressiveness,
      sldrThis.coverCheck, sldrThis.searchRadius, sldrThis.dodgeCheck, sldrThis.hearingRadius,
      sldrThis.intraBurstShotTime, sldrThis.intraBurstShotVariation, grenadeData, shieldData,
      flags0, flags1);

  return rs_new CCommandoPirate(mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
                                LdrToEntityInfo(info, sldrThis.editorProperties),
                                LdrToTransform4f(sldrThis.editorProperties), *modelData,
                                LdrToActorParameters(sldrThis.actorInformation),
                                LdrToPatternedInfo(sldrThis.patterned, &sldrThis.ingPossessionData),
                                data);
}

static SCommandoPirate_FuncPtrs REL_loader_CommandoPirate;

// Retail's RELMain calls fn_9_138, which stores the loader fn_9_168 into lbl_9_bss_28 (the
// module's SCommandoPirate_FuncPtrs) and calls the DOL's SetSCommandoPirate_FuncPtrs. The proxy in
// SCommandoPirate_FuncPtrs.cpp calls through it.
void RELMain() {
  REL_loader_CommandoPirate.mLoader = PortLoadCommandoPirate;
  SetSCommandoPirate_FuncPtrs(&REL_loader_CommandoPirate);
}

void RELExit() { SetSCommandoPirate_FuncPtrs(nullptr); }
