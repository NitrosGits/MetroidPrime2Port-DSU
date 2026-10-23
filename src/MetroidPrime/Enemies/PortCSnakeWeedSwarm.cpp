// Host body for the SnakeWeedSwarm REL (module 71): MP1's CSnakeWeedSwarm (a field of boids that
// rise out of the ground and retreat when shot), ported onto Echoes' CActor. The analysis is
// docs/module-analysis/SnakeWeedSwarm.md; the retail function each method corresponds to is named on
// it. The REL's code is MP1's with these differences, each read from its disassembly:
// - Think starts with CActor::Think and has no rendering-model switch (MP1's x140_25 branch);
// - skinning goes through CSkinnedModelState (StoreCalculation/DrawFromState), Echoes' form of MP1's
//   position/normal workspaces;
// - the player in the weeds gets kSM_InShrubbery, and touching is one bit per player, each player
//   taking the contact damage in Think;
// - PreRender has no thermal-visor branch;
// - the particle generators are never built: the loader passes invalid particle ids and a zero
//   count, so the retreat/continuous particle helpers (fn_71_7F0/fn_71_880) are left out.
#include "MetroidPrime/Enemies/CSnakeWeedSwarm.hpp"

#include "Collision/CMaterialFilter.hpp"
#include "Collision/CMaterialList.hpp"
#include "Collision/CRayCastResult.hpp"
#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CResFactory.hpp"
#include "Kyoto/Graphics/CGraphics.hpp"
#include "Kyoto/Graphics/CModelFlags.hpp"
#include "Kyoto/Math/CFrustumPlanes.hpp"
#include "Kyoto/Math/CMath.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/CActorLights.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAnimRes.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrSnakeWeedSwarm.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"
#include "MetroidPrime/Weapons/WeaponTypes.hpp"

#include <math.h>
#include <stdio.h>

namespace {

ushort TranslateSfx(ushort id) {
  return CSfxManager::TranslateSFXID(id == 0xffff ? CSfxManager::kInternalInvalidSfxId : id);
}

} // namespace

CSnakeWeedSwarm::CBoid::CBoid(const CVector3f& pos, float depth, float speed, float scale)
: x0_pos(pos)
, xc_state(kSWBS_Raising)
, x10_timeOut(0.f)
, x14_depth(depth)
, x18_speed(speed)
, x1c_(0.f)
, x20_scale(scale) {}

// fn_71_2B84 (0xB54). The CActor base gets (uid, name, info, 0, Translate(pos), CModelData(animRes),
// CMaterialList(kMT_Trigger, kMT_NonSolidDamageable), actParms, kInvalidUniqueId): the material
// list is built from lbl_71_data_4 (0x22 = kMT_Trigger) and lbl_71_data_8 (0x32 =
// kMT_NonSolidDamageable). The rest is MP1's constructor: cos(maxSlope * pi/180) at 0x18c, the
// inverted touch bounds, the actor lights and four CModelData copies of the animRes.
CSnakeWeedSwarm::CSnakeWeedSwarm(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
                                 const CVector3f& pos, const CVector3f& scale,
                                 const CAnimRes& animRes, const CActorParameters& actParms,
                                 float spacing, float height, float variance,
                                 float weaponDamageRadius, float maxPlayerDistance,
                                 float loweredTime, float loweredTimeVariation, float maxDepth,
                                 float speed, float speedVariation, float slopeAngle,
                                 float scaleMin, float scaleMax, float distanceBelowGround,
                                 const CDamageInfo& damageInfo, float damageWaitTime,
                                 ushort sfxId1, ushort sfxId2, ushort sfxId3)
: CActor(uid, name, info, 0, CTransform4f::Translate(pos), CModelData(animRes),
         CMaterialList(kMT_Trigger, kMT_NonSolidDamageable), actParms, kInvalidUniqueId)
, xe8_scale(scale)
, xf4_boidSpacing(spacing)
, xf8_height(height)
, xfc_variance(variance)
, x100_weaponDamageRadius(weaponDamageRadius)
, x104_maxPlayerDistance(maxPlayerDistance)
, x108_loweredTime(loweredTime)
, x10c_loweredTimeVariation(loweredTimeVariation)
, x110_maxDepth(maxDepth)
, x114_speed(speed)
, x118_speedVariation(speedVariation)
, x11c_cosSlopeAngle(static_cast< float >(cos(slopeAngle * 0.017453292f)))
, x120_scaleMin(scaleMin)
, x124_scaleMax(scaleMax)
, x128_distanceBelowGround(distanceBelowGround)
, mDamageWaitTime(damageWaitTime)
, x12c_(0)
, x140_24_hasGround(false)
, x140_25_modelAssetDirty(false)
, mPlayersTouching(0)
, x144_touchBounds(CAABox::MakeMaxInvertedBox())
, x15c_damageInfo(damageInfo)
, x1c4_which(CModelData::kWM_Normal)
, x1c8_boidPositions(rs_new rstl::vector< CVector3f >)
, x1cc_boidPlacement(rs_new rstl::vector< EBoidPlacement >)
, x1d0_sfx1(TranslateSfx(sfxId1))
, x1d2_sfx2(TranslateSfx(sfxId2))
, x1d4_sfx3(TranslateSfx(sfxId3))
, x1d8_sfxHandle()
, mThinkCount(0) {
  SetActorLights(actParms.GetLighting().MakeActorLights());
  for (int i = 0; i < 4; ++i) {
    x1b0_modelData.push_back(rstl::ncrc_ptr< CModelData >(rs_new CModelData(animRes)));
  }
}

// fn_71_2810 (0x1B0, vtable slot 2).
CSnakeWeedSwarm::~CSnakeWeedSwarm() {}

// vtable slot 6 = fn_71_274C (0xC4): on kSM_Create (MP1's Registered) the skinned models are
// allocated (fn_71_256C), lighting is turned on and the lights cast shadows; on kSM_Delete the
// looped emitter is removed.
void CSnakeWeedSwarm::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  CActor::AcceptScriptMsg(mgr, msg);
  switch (msg.GetMessage()) {
  case kSM_Create:
    AllocateSkinnedModels(mgr, CModelData::kWM_Normal);
    SetCalculateLighting(true);
    if (ActorLights()) {
      ActorLights()->SetCastShadows(true);
    }
    break;
  case kSM_Delete:
    if (x1d8_sfxHandle) {
      CSfxManager::RemoveEmitter(x1d8_sfxHandle);
      x1d8_sfxHandle = CSfxHandle();
    }
    break;
  default:
    break;
  }
}

// fn_71_256C: MP1's InitAnimBoids. Each of the four models starts a quarter of its "Whole Body"
// animation further on, so neighbouring weeds do not sway in step.
void CSnakeWeedSwarm::AllocateSkinnedModels(CStateManager& mgr, CModelData::EWhichModel which) {
  mModelStates.clear();
  for (int i = 0; i < 4; ++i) {
    CModelData& data = *x1b0_modelData[i];
    mModelStates.push_back(data.PickAnimatedModel(which).MakeDefaultStorage());
    data.EnableLooping(true);
    data.AdvanceAnimation(
        data.GetAnimationData()->GetAnimTimeRemaining(rstl::string_l("Whole Body")) *
            (static_cast< float >(i) / 4.f),
        mgr, GetCurrentAreaId(), true);
  }
  x1c4_which = which;
}

CAABox CSnakeWeedSwarm::GetBoundingBox() const {
  const CVector3f scale(0.75f * xe8_scale.GetX(), 0.75f * xe8_scale.GetY(),
                        0.75f * xe8_scale.GetZ());
  return CAABox(GetTranslation() - scale, GetTranslation() + scale);
}

// vtable slot 18 = fn_71_2494 (0x54): the touch bounds once the swarm has found ground.
rstl::optional_object< CAABox > CSnakeWeedSwarm::GetTouchBounds() const {
  if (!x140_24_hasGround) {
    return rstl::optional_object< CAABox >();
  }
  return x144_touchBounds;
}

void CSnakeWeedSwarm::UpdateTouchBounds() {
  if (x130_boids.size() > 0) {
    x144_touchBounds = CAABox::MakeMaxInvertedBox();
    const float radius = x100_weaponDamageRadius;
    for (rstl::vector< CBoid >::const_iterator it = x130_boids.begin(); it != x130_boids.end();
         ++it) {
      x144_touchBounds.AccumulateBounds(it->GetLocation() - CVector3f(radius, radius, radius));
      x144_touchBounds.AccumulateBounds(it->GetLocation() + CVector3f(radius, radius, radius));
    }
  } else {
    x144_touchBounds = CAABox(GetTranslation(), GetTranslation());
  }
  SetTransformDirty(true);
}

// vtable slot 5 = fn_71_1E58 (0x4D4).
void CSnakeWeedSwarm::Think(float dt, CStateManager& mgr) {
  CActor::Think(dt, mgr);
  if (!GetActive()) {
    return;
  }
  if (!x140_24_hasGround) {
    CreateSwarm(mgr);
  }
  if (x140_24_hasGround && x1c8_boidPositions.get() && x1c8_boidPositions->size() > 0) {
    FloodFill(mgr, static_cast< int >(dt * x1cc_boidPlacement->size() * 1.f) + 1);
  }
  if (mModelStates.empty()) {
    // Host guard: kSM_Create allocates the skinned models; never draw or advance without them.
    return;
  }
  for (int i = 0; i < 4; ++i) {
    x1b0_modelData[i]->AnimationData()->SetPlaybackRate(1.f);
    x1b0_modelData[i]->AdvanceAnimation(dt, mgr, GetCurrentAreaId(), true);
  }
  int raisedBoids = 0;
  for (rstl::vector< CBoid >::iterator it = x130_boids.begin(); it != x130_boids.end(); ++it) {
    CBoid& boid = *it;
    switch (boid.GetBoidState()) {
    case kSWBS_Raised:
      ++raisedBoids;
      break;
    case kSWBS_Raising:
      boid.SetDepth(boid.GetDepth() - dt * boid.GetSpeed());
      if (boid.GetDepth() <= 0.f) {
        boid.SetDepth(0.f);
        boid.SetBoidState(kSWBS_Raised);
      }
      break;
    case kSWBS_Lowered:
      boid.SetTimeOut(boid.GetTimeOut() - dt);
      if (boid.GetTimeOut() <= 0.f) {
        boid.SetBoidState(kSWBS_Raising);
        CSfxManager::AddEmitter(x1d4_sfx3, boid.GetLocation(), GetCurrentAreaId().Value(), true,
                                false, CSfxManager::kMedPriority);
      }
      break;
    case kSWBS_Lowering:
      boid.SetDepth(boid.GetDepth() + dt * boid.GetSpeed());
      if (boid.GetDepth() > x110_maxDepth * boid.GetSize()) {
        boid.SetDepth(x110_maxDepth * boid.GetSize());
        boid.SetTimeOut(x10c_loweredTimeVariation * mgr.Random()->Float() + x108_loweredTime);
        boid.SetBoidState(kSWBS_Lowered);
      }
      break;
    }
  }
  if (raisedBoids > 0) {
    if (!x1d8_sfxHandle) {
      x1d8_sfxHandle = CSfxManager::AddEmitter(x1d0_sfx1, GetTranslation(),
                                               GetCurrentAreaId().Value(), true, true,
                                               CSfxManager::kMedPriority);
    }
  } else if (x1d8_sfxHandle) {
    CSfxManager::RemoveEmitter(x1d8_sfxHandle);
    x1d8_sfxHandle = CSfxHandle();
  }
  // Echoes: every player the weeds caught in Touch takes the contact damage, scaled for the frame.
  for (int i = 0; i < static_cast< int >(mgr.GetNumPlayers()); ++i) {
    CPlayer* player = mgr.GetPlayer(i);
    if (player == nullptr ||
        (mPlayersTouching & (1 << mgr.MaskUIdNumPlayers(player->GetUniqueId()))) == 0) {
      continue;
    }
    mgr.ApplyDamage(
        GetUniqueId(), player->GetUniqueId(), GetUniqueId(), CDamageInfo(x15c_damageInfo, dt),
        CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList()),
        CVector3f::Zero());
  }
  mPlayersTouching = 0;
  // Port: a line at the 300th active update says the swarm is thinking and how many weeds are up.
  if (++mThinkCount == 300) {
    fprintf(stderr, "[snakeweed] %04x thinking: %d of %d weeds raised\n", GetUniqueId().Value(),
            raisedBoids, static_cast< int >(x130_boids.size()));
  }
}

// vtable slot 19 = fn_71_1CC0 (0x198): a non-AI projectile scares the weeds within the weapon
// damage radius; a player near a raised weed is told it is in the shrubbery and marked touching.
void CSnakeWeedSwarm::Touch(CActor& actor, CStateManager& mgr) {
  if (CGameProjectile* projectile = TCastToPtr< CGameProjectile >(actor)) {
    if (projectile->GetCurrentDamageInfo().GetWeaponMode().GetType() != kWT_AI) {
      ScareSnakeWeeds(mgr, projectile->GetTranslation(), x100_weaponDamageRadius);
    }
  }
  if (CPlayer* player = TCastToPtr< CPlayer >(actor)) {
    for (rstl::vector< CBoid >::const_iterator it = x130_boids.begin(); it != x130_boids.end();
         ++it) {
      if ((it->GetLocation() - player->GetTranslation()).Magnitude() < x104_maxPlayerDistance &&
          it->GetBoidState() == kSWBS_Raised) {
        mgr.SendScriptMsg(player, kInvalidUniqueId, kSM_InShrubbery);
        mPlayersTouching |= 1 << mgr.MaskUIdNumPlayers(player->GetUniqueId());
        return;
      }
    }
  }
}

// fn_71_1B3C (0x184): lowers every raised or rising boid within the radius.
void CSnakeWeedSwarm::ScareSnakeWeeds(CStateManager& mgr, const CVector3f& pos, float radius) {
  const float radiusSquared = radius * radius;
  for (rstl::vector< CBoid >::iterator it = x130_boids.begin(); it != x130_boids.end(); ++it) {
    CBoid& boid = *it;
    if ((boid.GetLocation() - pos).MagSquared() < radiusSquared &&
        (boid.GetBoidState() == kSWBS_Raised || boid.GetBoidState() == kSWBS_Raising)) {
      boid.SetBoidState(kSWBS_Lowering);
      boid.SetSpeed(x118_speedVariation * mgr.Random()->Float() + x114_speed);
      CSfxManager::AddEmitter(x1d2_sfx2, boid.GetLocation(), GetCurrentAreaId().Value(), true,
                              false, CSfxManager::kMedPriority);
    }
  }
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

// vtable slot 9 = fn_71_1934 (0x1C4): MP1's PreRender without the thermal branch. Lights rebuild
// when the actor moved or its lights ask for a relight.
void CSnakeWeedSwarm::PreRender(CStateManager& mgr) {
  if (!mgr.GetFrustumPlanes().BoxInFrustumPlanes(x144_touchBounds)) {
    SetPreRenderClipped(true);
    return;
  }
  SetPreRenderClipped(false);
  if (mModelStates.empty()) {
    return;
  }
  for (int i = 0; i < 4; ++i) {
    x1b0_modelData[i]->AnimationData()->PreRender();
  }
  if (ActorLights() && x140_24_hasGround) {
    bool buildLights = false;
    if (GetPreRenderHasMoved()) {
      buildLights = true;
      SetPreRenderHasMoved(false);
    } else if (ActorLights()->GetNeedsRelight()) {
      buildLights = true;
    }
    if (GetCalculateLighting()) {
      const CAABox bounds = x144_touchBounds;
      if (buildLights && GetCurrentAreaId() != kInvalidAreaId) {
        CWorld* world = mgr.World();
        if (world->IsAreaValid(GetCurrentAreaId())) {
          ActorLights()->BuildAreaLightList(mgr, *world->GetArea(GetCurrentAreaId()), bounds);
        }
      }
      ActorLights()->BuildDynamicLightList(mgr, bounds);
    }
  }
}

// fn_71_17C0: builds and stores the pose of the boid's model once per frame (one bit per model in
// posesToBuild), then draws it sunk by the boid's depth and scaled by its size.
void CSnakeWeedSwarm::RenderBoid(uint index, const CBoid& boid, uint& posesToBuild,
                                 const CModelFlags& flags) const {
  const uint modelIndex = index & 3;
  uint mask = posesToBuild;
  const CModelData& data = *x1b0_modelData[modelIndex];
  const CAnimData& animData = *data.GetAnimationData();
  CSkinnedModel& model = data.PickAnimatedModel(x1c4_which);
  CSkinnedModelState& state = const_cast< CSkinnedModelState& >(mModelStates[modelIndex]);
  const uint bit = 1 << modelIndex;
  if (mask & bit) {
    mask &= ~bit;
    animData.BuildPose();
    model.StoreCalculation(state, &animData.Pose());
  }
  gpRender->SetModelMatrix(
      CTransform4f::Translate(boid.GetLocation() - CVector3f(0.f, 0.f, boid.GetDepth())) *
      CTransform4f::Scale(boid.GetSize()));
  model.DrawFromState(state, flags);
  posesToBuild = mask;
}

// vtable slot 10 = fn_71_163C (0x184).
void CSnakeWeedSwarm::AddToRenderer(const CStateManager& mgr) const {
  if (GetPreRenderClipped() || mModelStates.empty()) {
    return;
  }
  uint posesToBuild = 0xffffffff;
  if (GetActorLights()) {
    GetActorLights()->ActivateLights();
  } else {
    CGraphics::DisableAllLights();
    gpRender->SetAmbientColor(CColor::White());
  }
  const int alpha = GetRenderAlphaBufferAlpha(mgr);
  if (alpha != -1) {
    gpRender->SetDestinationAlpha(alpha);
  }
  const CModelFlags flags(CModelFlags::kT_Opaque, 0,
                          static_cast< CModelFlags::EFlags >(CModelFlags::kF_DepthCompare |
                                                             CModelFlags::kF_DepthUpdate),
                          CColor(1.f, 1.f, 1.f, 1.f));
  uint index = 0;
  for (rstl::vector< CBoid >::const_iterator it = x130_boids.begin(); it != x130_boids.end();
       ++it, ++index) {
    RenderBoid(index, *it, posesToBuild, flags);
  }
  CGraphics::DisableAllLights();
  if (alpha != -1) {
    gpRender->DisableDestinationAlpha();
  }
}

// fn_71_133C: a ray down the bounding box finds the ground the flood fill starts from.
void CSnakeWeedSwarm::CreateSwarm(CStateManager& mgr) {
  const CMaterialFilter filter = CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid));
  const CAABox box = GetBoundingBox();
  const CVector3f pos = box.GetCenterPoint();
  const CVector3f down(0.f, 0.f, -1.f);
  const CRayCastResult result = mgr.RayStaticIntersection(
      pos, down, box.GetMaxPoint().GetZ() - box.GetMinPoint().GetZ(), filter);
  if (result.IsValid()) {
    x130_boids.reserve(GetGridWidth() * GetGridDepth());
    x1c8_boidPositions->reserve(GetGridWidth() * GetGridDepth());
    x1cc_boidPlacement->resize(GetGridWidth() * GetGridDepth(), kBP_None);
    x1c8_boidPositions->push_back(result.GetPoint());
    x140_24_hasGround = true;
  }
}

CVector2i CSnakeWeedSwarm::GetGridPosition(CVector3f pos) {
  const CAABox box = GetBoundingBox();
  return CVector2i(static_cast< int >((pos.GetX() - box.GetMinPoint().GetX()) / xf4_boidSpacing),
                   static_cast< int >((pos.GetY() - box.GetMinPoint().GetY()) / xf4_boidSpacing));
}

int CSnakeWeedSwarm::GetGridWidth() {
  const CAABox box = GetBoundingBox();
  return static_cast< int >((box.GetMaxPoint().GetX() - box.GetMinPoint().GetX()) /
                            xf4_boidSpacing) +
         1;
}

int CSnakeWeedSwarm::GetGridDepth() {
  const CAABox box = GetBoundingBox();
  return static_cast< int >((box.GetMaxPoint().GetY() - box.GetMinPoint().GetY()) /
                            xf4_boidSpacing) +
         1;
}

void CSnakeWeedSwarm::PushBackPoint(CVector3f pos) {
  const int width = GetGridWidth();
  const int depth = GetGridDepth();
  const CVector2i grid = GetGridPosition(pos);
  if (grid[0] < 0 || grid[0] >= width || grid[1] < 0 || grid[1] >= depth) {
    return;
  }
  const int index = grid[0] + width * grid[1];
  if ((*x1cc_boidPlacement)[index] == kBP_None) {
    (*x1cc_boidPlacement)[index] = kBP_Ready;
    x1c8_boidPositions->push_back(pos);
  }
}

// fn_71_D10: places up to count weeds from the pending points, spreading to the four neighbours of
// every point that took one.
void CSnakeWeedSwarm::FloodFill(CStateManager& mgr, int count) {
  const int width = GetGridWidth();
  for (int i = 0; i < count && x1c8_boidPositions->size() > 0; ++i) {
    const CVector3f pos = x1c8_boidPositions->back();
    x1c8_boidPositions->pop_back();
    const CVector2i grid = GetGridPosition(pos);
    if (PlaceSnakeWeedAtPoint(pos, mgr)) {
      (*x1cc_boidPlacement)[grid[0] + width * grid[1]] = kBP_Placed;
      PushBackPoint(CVector3f(pos.GetX(), pos.GetY() - xf4_boidSpacing, pos.GetZ()));
      PushBackPoint(CVector3f(pos.GetX(), xf4_boidSpacing + pos.GetY(), pos.GetZ()));
      PushBackPoint(CVector3f(pos.GetX() - xf4_boidSpacing, pos.GetY(), pos.GetZ()));
      PushBackPoint(CVector3f(xf4_boidSpacing + pos.GetX(), pos.GetY(), pos.GetZ()));
    } else {
      (*x1cc_boidPlacement)[grid[0] + width * grid[1]] = kBP_Invalid;
    }
  }
  UpdateTouchBounds();
  if (x1c8_boidPositions->size() == 0) {
    // Port: one line per swarm when its field is complete, the evidence a run reads.
    fprintf(stderr, "[snakeweed] %04x placed %d weeds\n", GetUniqueId().Value(),
            static_cast< int >(x130_boids.size()));
    x1c8_boidPositions = nullptr;
    x1cc_boidPlacement = nullptr;
  }
}

float CSnakeWeedSwarm::GetXVariance(const CVector3f& pos) {
  const float f = 8.21395f * pos.GetX() + 0.112869f * (pos.GetY() * pos.GetY());
  return xfc_variance * (2.f * CMath::AbsF(f - static_cast< int >(f)) - 1.f);
}

float CSnakeWeedSwarm::GetYVariance(const CVector3f& pos) {
  const float f = 2.4729404f * pos.GetY() + 0.3478602f * (pos.GetX() * pos.GetX());
  return xfc_variance * (2.f * CMath::AbsF(f - static_cast< int >(f)) - 1.f);
}

bool CSnakeWeedSwarm::PlaceSnakeWeedAtPoint(const CVector3f& point, CStateManager& mgr) {
  const CMaterialFilter filter = CMaterialFilter::MakeInclude(CMaterialList(kMT_Solid));
  const CVector3f pos = point + CVector3f(GetXVariance(point), GetYVariance(point), xf8_height);
  const CVector3f down(0.f, 0.f, -1.f);
  const CRayCastResult result = mgr.RayStaticIntersection(pos, down, 2.f * xf8_height, filter);
  if (result.IsValid() && CVector3f::Dot(result.GetPlane().GetNormal(), CVector3f(0.f, 0.f, 1.f)) >
                              x11c_cosSlopeAngle) {
    const CVector3f boidPos = result.GetPoint() - CVector3f(0.f, 0.f, x128_distanceBelowGround);
    const CBoid boid(boidPos, x110_maxDepth, x114_speed + x118_speedVariation,
                     (x124_scaleMax - x120_scaleMin) * mgr.Random()->Float() + x120_scaleMin);
    x130_boids.push_back(boid);
    return true;
  }
  return false;
}

// Retail fn_71_69C: SLdrSnakeWeedSwarm's constructor, the loader's defaults (the call at fn_71_DC
// 0xFC, on the loader's stack object). The generated header's inline constructor stores the same
// floats and damage info, except the sound ids: the guest's default for every sound id is -1 (no
// sound), the header's 0. Those -1 stores are written over it here. The generated struct is 0xC
// bytes larger than the guest's from the actor-parameters block on, so guest 0x10c is the port's
// sound_Looped (0x118): the three stores the `li r0,-0x1` at 0x000006CC feeds, 0x10c
// (0x000006D4), 0x110 (0x000006E4) and 0x114 (0x000006F0). A property in the instance's data
// overwrites any of them.
static void x69C_SoundDefaults(SLdrSnakeWeedSwarm& sldr) {
  sldr.sound_Looped = -1;      // 0x000006D4 (0x10c)
  sldr.sound_IntoGround = -1;  // 0x000006E4 (0x110)
  sldr.sound_OutofGround = -1; // 0x000006F0 (0x114)
}

// Retail fn_71_DC (0x544). Property ids and order are the generated SLdrSnakeWeedSwarm reader's;
// the REL's switch compares the same 22 ids. The REL then builds a CAnimRes on its stack (ancs,
// character, scale 1,1,1, initial animation, loop true) and constructs the 0x298-byte object.
static CEntity* PortLoadSnakeWeedSwarm(CStateManager& mgr, CInputStream& input, CEntityInfo& info) {
  SLdrSnakeWeedSwarm sldrThis;
  x69C_SoundDefaults(sldrThis);  // fn_71_69C's -1 sound ids (0xFC)
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
