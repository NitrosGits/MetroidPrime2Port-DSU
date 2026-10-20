// The Ing REL's (module 29) CIngExitHostEffect, ported for behaviour rather than bytes: the swarm
// effect CIng's ExitHost state spawns (ctor fn_29_1105C, dtor fn_29_FF88). Its Think,
// AcceptScriptMsg, AddToRenderer and Render are not ported yet, and nothing has run this class, so
// no `// @` markers. The member map is docs/module-analysis/IngFamily.md section A.9.
//
// fn_29_11490 is a plain `CModelData()`, inlined below as the base class's model argument.
// fn_29_11438 is the copy-assignment of retail's inline [size][60 x CVector3f] trail container; the
// ctor only ever fills it with 60 copies of the actor's translation, so it is done in the ctor body
// rather than as a function.
// fn_29_10D50 is the child-particle-system builder both token members go through (once per asset).

#include "MetroidPrime/Enemies/CIngExitHostEffect.hpp"

#include "Collision/CMaterialList.hpp"
#include "Kyoto/CResFactory.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "Kyoto/Particles/CParticleGen.hpp"
#include "Kyoto/SObjectTag.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CModelData.hpp"

namespace {

// The ctor's asset-to-token step, inlined twice in retail: the resource type comes from the loader,
// then the pool hands back a token. It stays a plain CToken because asset A is an SRSC and asset B a
// PART; fn_29_10D50 converts.
CToken LoadAssetToken(CAssetId asset) {
  return gpSimplePool->GetObj(SObjectTag(gpResourceFactory->GetResourceTypeById(asset), asset));
}

// fn_29_10D50 (not yet exercised in a run)
// Builds the effect from a token and a scale: CElementGen::ConstructChildParticleSystem with the
// token's own reference type (retail reads `token.mObjRef->mObjTag.type`), seed 0, kOSF_One, no
// model lights, emitting, the actor's translation (retail 0x54), identity orientation, zero global
// translation, the scale, white and a unit local scale. Its result's sign is the "created" byte in
// retail, i.e. just whether this is non-null; a token with no reference is the one case where
// retail would fault, so the host skips it.
CParticleGen* ConstructEffect(CToken& token, const CVector3f& scale, const CVector3f& translation) {
  if (!token.HasReference()) {
    return nullptr;
  }
  return CElementGen::ConstructChildParticleSystem(
      token, token.GetReferenceType(), 0, CElementGen::kOSF_One, false, true, translation,
      CTransform4f::Identity(), CVector3f::Zero(), CTransform4f::Identity(), scale,
      CColor::White(), CVector3f::One());
}

} // namespace

// fn_29_1105C (not yet exercised in a run)
// Base: CActor(uid, "IngExitHostSwarmFx", info, 0, xf, CModelData(), CMaterialList(),
// CActorParameters(), kInvalidUniqueId) - retail's material list is empty (two zero words), unlike
// the mini portal's single kMT_Projectile. The name is rodata lbl_29_rodata_7C8.
CIngExitHostEffect::CIngExitHostEffect(TUniqueId uid, const CEntityInfo& info,
                                       const CTransform4f& xf, const CVector3f& dest,
                                       const CVector3f& scale, CAssetId assetA, CAssetId assetB,
                                       float x2c4, float x290, TUniqueId target, float x298,
                                       float x2a0, ushort sound)
: CActor(uid, rstl::string_l("IngExitHostSwarmFx"), info, 0, xf, CModelData(), CMaterialList(),
         CActorParameters(), kInvalidUniqueId)
, mXf(xf) // 0x158: a copy of the ctor's transform, CTransform4f's copy ctor
// 0x188: (null area, flags 3, index 0, 1.0f, 1.0f, no avoidance filter, kRP_Center); rodata_64,
// which the ctor loads for both radii, is 1.0f.
, mPathFindSearch(nullptr, 3, 0, 1.f, 1.f, 0, CPFRegion::kRP_Center)
, mDest(dest)               // 0x274
, mScale(scale)             // 0x280
, mX28c(0.f)                // 0x28c
, mX290(x290)               // 0x290
, mTarget(target)           // 0x294
, mX298(x298)               // 0x298
, mX29c(x298)               // 0x29c, the same value again
, mX2a0(x2a0)               // 0x2a0
, mTokenA(LoadAssetToken(assetA)) // 0x2a4
, mEffectA(nullptr)               // 0x2b0
, mTokenB(LoadAssetToken(assetB)) // 0x2b4
, mEffectB(nullptr)               // 0x2c0
, mX2c4(x2c4)                     // 0x2c4
, mBezierCount(0)                 // 0x2c8
, mX2fc(0.f)                      // 0x2fc
, mX300(1.f)                      // 0x300
, mTrailIndex(0)                  // 0x5d8
, mSwarmMoveHandle()              // 0x5dc, null
, mSound(sound)                   // 0x5e0
, mFlags(0)                       // 0x5e2
{
  // 0x2cc..0x2f8: the four bezier control points, zero here (the array member cannot be
  // value-initialised in the initialiser list).
  mBezier[0] = CVector3f::Zero();
  mBezier[1] = CVector3f::Zero();
  mBezier[2] = CVector3f::Zero();
  mBezier[3] = CVector3f::Zero();

  // The two effects, A then B, both from the actor's translation and the model scale.
  mEffectA = ConstructEffect(mTokenA, scale, GetTranslation());
  mEffectB = ConstructEffect(mTokenB, scale, GetTranslation());

  // fn_29_1105C ends by calling retail slot 0x34 on mEffectA only - CParticleGen::SetGeneratorRate.
  if (mEffectA) {
    mEffectA->SetGeneratorRate(1.f);
  }

  // 0x5e2: retail clears bit 7 and leaves the other bits as they were, i.e. read from uninitialised
  // memory, which the host does not do (zero above); the effect's Think sets bit 7 once it has
  // arrived, and ExitHost's Update watcher reads it.
  //
  // 0x304/0x308: fn_29_11438 is retail's copy-assignment for the inline trail container - the ctor
  // builds 60 copies of the actor's translation (retail 0x54) in a temporary and copies them in.
  const CVector3f translation = GetTranslation();
  for (int i = 0; i < 60; ++i) {
    mTrail[i] = translation;
  }
}

// fn_29_FF88 (not yet exercised in a run)
// Retail resets the vtable, deletes each effect through CParticleGen's deleting destructor (slot
// 0x08) when its pointer is non-null, then destroys the two tokens and the CActor base in reverse.
CIngExitHostEffect::~CIngExitHostEffect() {
  delete mEffectB;
  delete mEffectA;
}
