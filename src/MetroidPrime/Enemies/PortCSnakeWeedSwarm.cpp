// Host body for the SnakeWeedSwarm REL (module 71), milestone 1: the actors load, think and render.
// The analysis is docs/module-analysis/SnakeWeedSwarm.md. The class is MP1's CSnakeWeedSwarm on
// Echoes' CActor (a field of boids that rise out of the ground and retreat), but no boid exists
// yet: the swarm creation, the flood fill that places the weeds, the boid state machine, the
// per-boid skinned rendering and the particles are later milestones, so the overrides below that
// would run them forward to CActor behind a once-per-method StandIn log instead. The retail
// function each one stands in for is named on it.
//
// Which methods CSnakeWeedSwarm overrides is not guessed: the REL's vtable lbl_71_data_30 lists
// them by address, and every slot it inherits keeps the DOL's own symbol (slot 4 PreThink is
// CEntity's, slot 11 Render is CActor's, so retail never renders the actor's own model - its
// AddToRenderer draws the boids directly). Bodies short enough to be decided from the disassembly
// (GetTouchBounds, ApplyRadiusDamage, the kSM_Delete arm of AcceptScriptMsg) are real.
#include "MetroidPrime/Enemies/CSnakeWeedSwarm.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CResFactory.hpp"
#include "MetroidPrime/CAnimRes.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSnakeWeedSwarm.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"

#include <math.h>
#include <stdio.h>

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
  fprintf(stderr, "[snakeweed] %s is a port stand-in (see PortCSnakeWeedSwarm.cpp)\n", what);
}

} // namespace

// fn_71_2B84 (0xB54). The CActor base gets (uid, name, info, 0, Translate(pos), CModelData(animRes),
// CMaterialList(kMT_Trigger, kMT_NonSolidDamageable), actParms, kInvalidUniqueId): the material
// list is built from lbl_71_data_4 (0x22 = kMT_Trigger) and lbl_71_data_8 (0x32 =
// kMT_NonSolidDamageable) with 0x3b (kMT_Solid, the list's end marker) in the unused slots, and r7
// is li 0. The ctor then stores the scale at 0x158, the floats at 0x164..0x198 (0x18c is
// cos(maxSlope * pi/180), lbl_71_rodata_50 = 0.017453292), zeroes the boid vector and the bounds
// flags, starts the touch bounds from CAABox::mskInvertedBox and builds MakeActorLights from the
// actor parameters' light parameters (SetActorLights). The four per-boid CModelData copies, the
// boid vectors and the particle generators are milestone 2.
CSnakeWeedSwarm::CSnakeWeedSwarm(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                                 const CVector3f& pos, const CVector3f& scale,
                                 const CAnimRes& animRes, const CActorParameters& actParms,
                                 float density, float maxDepth, float locationVariance,
                                 float detectionRadius, float grabRadius, float waitTimeOut,
                                 float waitTimeOutVariance, float retreatDepth, float moveSpeed,
                                 float moveSpeedVariance, float maxSlope, float minSize,
                                 float maxSize, float heightOffset,
                                 const CDamageInfo& contactDamage, float damageWaitTime,
                                 ushort sfxLooped, ushort sfxIntoGround, ushort sfxOutOfGround)
: CActor(uid, name, info, 0, CTransform4f::Translate(pos), CModelData(animRes),
         CMaterialList(kMT_Trigger, kMT_NonSolidDamageable), actParms, kInvalidUniqueId)
, mScale(scale)
, mDensity(density)
, mMaxDepth(maxDepth)
, mLocationVariance(locationVariance)
, mDetectionRadius(detectionRadius)
, mGrabRadius(grabRadius)
, mWaitTimeOut(waitTimeOut)
, mWaitTimeOutVariance(waitTimeOutVariance)
, mRetreatDepth(retreatDepth)
, mMoveSpeed(moveSpeed)
, mMoveSpeedVariance(moveSpeedVariance)
, mCosMaxSlope(static_cast< float >(cos(maxSlope * 0.017453292f)))
, mMinSize(minSize)
, mMaxSize(maxSize)
, mHeightOffset(heightOffset)
, mHasGround(false)
, mTouchBounds(CAABox::MakeMaxInvertedBox())
, mContactDamage(contactDamage)
, mDamageWaitTime(damageWaitTime)
, mSfxLooped(sfxLooped)
, mSfxIntoGround(sfxIntoGround)
, mSfxOutOfGround(sfxOutOfGround)
, mSfxHandle() {}

// fn_71_2810 (0x1B0, vtable slot 2): frees the tokens, element generators, boid vectors and model
// slots this milestone does not hold, then ~CActor.
CSnakeWeedSwarm::~CSnakeWeedSwarm() {}

// vtable slot 5 = fn_71_1E58 (0x4D4). CActor::Think is its first call [proven]; the rest creates the
// swarm on first use (fn_71_133C / fn_71_D10, the grid flood fill), advances the animation, drives
// each boid's raise/lower timers and plays the into/out-of-ground sounds.
void CSnakeWeedSwarm::Think(float dt, CStateManager& mgr) {
  StandIn("CSnakeWeedSwarm::Think (vtable slot 5, fn_71_1E58)");
  CActor::Think(dt, mgr);
}

// vtable slot 6 = fn_71_274C (0xC4). After CActor::AcceptScriptMsg it handles two messages: on
// 'XCRT' (kSM_Create) it calls fn_71_256C(mgr, 0) - the skinned-model allocation, MP1's
// AllocateSkinnedModels - then SetCalculateLighting(true) and sets a cast-shadow bit in the actor
// lights (+0x2a0 bit 0x40); on 'XDEL' (kSM_Delete) it removes the looped emitter at 0x264.
void CSnakeWeedSwarm::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CActor::AcceptScriptMsg(mgr, msg);
  switch (msg.GetMessage()) {
  case kSM_Create:
    StandIn("CSnakeWeedSwarm::AllocateSkinnedModels (fn_71_256C, from kSM_Create)");
    SetCalculateLighting(true);
    break;
  case kSM_Delete:
    if (mSfxHandle) {
      CSfxManager::RemoveEmitter(mSfxHandle);
      mSfxHandle = CSfxHandle();
    }
    break;
  default:
    break;
  }
}

// vtable slot 9 = fn_71_1934 (0x1C4): a frustum test of the boids' bounds, CAnimData::PreRender and
// the area/dynamic light lists - it does not call CActor::PreRender.
void CSnakeWeedSwarm::PreRender(CStateManager& mgr) {
  StandIn("CSnakeWeedSwarm::PreRender (vtable slot 9, fn_71_1934)");
  CActor::PreRender(mgr);
}

// vtable slot 10 = fn_71_163C (0x184): activates the actor lights and draws each boid through
// fn_71_17C0 (MP1's RenderBoid) at the render-alpha of GetRenderAlphaBufferAlpha. With no boids the
// stand-in draws the actor's own model at the swarm's origin, so the actor is visible.
void CSnakeWeedSwarm::AddToRenderer(const CStateManager& mgr) const {
  StandIn("CSnakeWeedSwarm::AddToRenderer (vtable slot 10, fn_71_163C)");
  CActor::AddToRenderer(mgr);
}

// vtable slot 18 = fn_71_2494 (0x54), whole body: the touch bounds at 0x1b4 when bit 0x80 of the
// byte at 0x1b0 is set, otherwise none. The bit is set once boids exist, so in this milestone the
// swarm has no touch bounds and Touch is never called.
rstl::optional_object< CAABox > CSnakeWeedSwarm::GetTouchBounds() const {
  if (!mHasGround) {
    return rstl::optional_object< CAABox >();
  }
  return mTouchBounds;
}

// vtable slot 19 = fn_71_1CC0 (0x198): projectiles scare the weeds (fn_71_1B3C), the player takes
// the contact damage through SendScriptMsg.
void CSnakeWeedSwarm::Touch(CActor& actor, CStateManager& mgr) {
  StandIn("CSnakeWeedSwarm::Touch (vtable slot 19, fn_71_1CC0)");
  CActor::Touch(actor, mgr);
}

// fn_71_1AF8 (0x44), whole body: a bomb or power bomb (the damage's weapon type, lhz 0x0, is 4 or 5)
// scares the weeds within the damage's radius (lfs 0xc).
void CSnakeWeedSwarm::ApplyRadiusDamage(CVector3f pos, const CDamageInfo& info,
                                        CStateManager& mgr) {
  const EWeaponType type = info.GetWeaponMode().GetType();
  if (type == kWT_Bomb || type == kWT_PowerBomb) {
    ScareSnakeWeeds(mgr, pos, info.GetRadius());
  }
}

// fn_71_1B3C (0x184): lowers every raised boid within the radius and plays the into-ground sound.
// There are no boids in this milestone.
void CSnakeWeedSwarm::ScareSnakeWeeds(CStateManager&, const CVector3f&, float) {
  StandIn("CSnakeWeedSwarm::ScareSnakeWeeds (fn_71_1B3C)");
}

// Retail fn_71_DC (0x544). Property ids and order are the generated SLdrSnakeWeedSwarm reader's;
// the REL's switch compares the same 22 ids. The REL then builds a CAnimRes on its stack (ancs,
// character, scale 1,1,1, initial animation, loop true) and constructs the 0x298-byte object.
static CEntity* PortLoadSnakeWeedSwarm(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrSnakeWeedSwarm sldrThis;
#include "MetroidPrime/ScriptLoader/SLdrSnakeWeedSwarm.inc"

  // Host guard (retail builds the CModelData unconditionally): an object with no character set
  // is skipped rather than given an empty animated model.
  if (gpResourceFactory->GetResourceTypeById(sldrThis.animationInformation.ancs) != 'ANCS') {
    fprintf(stderr, "[snakeweed] object has no ANCS (%08x); skipped\n",
            static_cast< uint >(sldrThis.animationInformation.ancs));
    return nullptr;
  }
  const CAnimRes animRes(sldrThis.animationInformation.ancs,
                         sldrThis.animationInformation.character_index, CVector3f(1.f, 1.f, 1.f),
                         sldrThis.animationInformation.initial_anim, true);

  return rs_new CSnakeWeedSwarm(
      mgr.AllocateUniqueId(), sldrThis.editorProperties.name,
      LdrToEntityInfo(info, sldrThis.editorProperties),
      sldrThis.editorProperties.transform.position, sldrThis.editorProperties.transform.scale,
      animRes, LdrToActorParameters(sldrThis.actorInformation), sldrThis.density,
      sldrThis.maxDepth, sldrThis.locationVariance, sldrThis.detectionRadius, sldrThis.grabRadius,
      sldrThis.waitTimeOut, sldrThis.waitTimeOutVariance, sldrThis.retreatDepth,
      sldrThis.moveSpeed, sldrThis.moveSpeedVariance, sldrThis.maxSlope, sldrThis.minSize,
      sldrThis.maxSize, sldrThis.heightOffset, LdrToDamageInfo(sldrThis.contactDamage),
      sldrThis.damageWaitTime, static_cast< ushort >(sldrThis.sound_Looped),
      static_cast< ushort >(sldrThis.sound_IntoGround),
      static_cast< ushort >(sldrThis.sound_OutofGround));
}

static SSnakeWeedSwarm_FuncPtrs REL_loader_SnakeWeedSwarm;

// Retail's RELMain calls fn_71_70, which stores the loader fn_71_DC at lbl_71_bss_40 +0x0 and copies
// the two pointers-to-member lbl_71_data_18 = {0, -1, fn_71_1AF8} and lbl_71_data_24 = {0, -1,
// fn_71_1B3C} (this delta, non-virtual, function) to +0x4 and +0x10, then calls the DOL's
// SetSSnakeWeedSwarm_FuncPtrs. CStateManager's radius damage reaches ApplyRadiusDamage through it
// (SnakeWeed_ApplyRadiusDamage in ScriptLoaderRel.cpp).
void RELMain() {
  REL_loader_SnakeWeedSwarm.mLoadSnakeWeedSwarm = PortLoadSnakeWeedSwarm;
  REL_loader_SnakeWeedSwarm.mApplyRadiusDamage =
      static_cast< void (CEntity::*)(CVector3f, const CDamageInfo&, CStateManager&) >(
          &CSnakeWeedSwarm::ApplyRadiusDamage);
  REL_loader_SnakeWeedSwarm.mScareSnakeWeeds =
      static_cast< void (CEntity::*)(CStateManager&, const CVector3f&, float) >(
          &CSnakeWeedSwarm::ScareSnakeWeeds);
  SetSSnakeWeedSwarm_FuncPtrs(&REL_loader_SnakeWeedSwarm);
}

void RELExit() { SetSSnakeWeedSwarm_FuncPtrs(nullptr); }
