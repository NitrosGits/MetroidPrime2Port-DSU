#ifndef _CMINORING
#define _CMINORING

#include "MetroidPrime/Enemies/CPatterned.hpp"
#include "REL/REL_Setup.h"

#ifdef TARGET_PC
#include "Kyoto/SObjectTag.hpp"
#include "MetroidPrime/CDamageInfo.hpp"
#include "MetroidPrime/CSurfaceAlignmentHelper.hpp"
#include "MetroidPrime/Enemies/CAiKnockBackMgr.hpp"
#include "MetroidPrime/Enemies/CIngSpotData.hpp"
#include "MetroidPrime/Enemies/CIngSpotPathFindNavigation.hpp"
#include "MetroidPrime/PathFinding/CPathFindPointSearch.hpp"
#include "MetroidPrime/PathFinding/CPathFindSearch.hpp"
#include "MetroidPrime/Weapons/CProjectileInfo.hpp"

// Host copy of the properties the MinorIng loader reads (retail: CMinorIngData, 0x158 bytes at
// 0x7c0, built by fn_44_9B4; the REL is module 44, `docs/module-analysis/IngFamily.md` section B).
// The guest offsets in the comments are the REL's own stores; the host layout is its own and does
// not need to match. Not modelled, because nothing in this milestone reads them: the 0x20 bytes at
// guest 0xb0 (fn_44_D00 fills them from a loader temporary), the 0x50 at guest 0xd0 (fn_44_C44
// copies the converted SLdrUnknownStruct34, i.e. `unknown_0x3da35851`) and the seven values at
// guest 0x120..0x138 that come out of that same struct.
struct CMinorIngData {
  CMinorIngData(CAssetId projectile, const CDamageInfo& projectileDamage,
                const CDamageInfo& ingSpotDamage, const CIngSpotData& ingSpot,
                float unknownA03E450C, float morphballPursuitDistance, float bombStunDuration,
                float ingSpotUnknown7569, float ingSpotSfxFallOff, float attackAngleLimit,
                float lineOfSightHeightOffset, float unknownD6C8EAC2, float unknown2A5449BA,
                float hearingRadius, float speedModifier, bool ingSpotAllowLockOn, uchar flags155,
                uchar flags156)
  : mProjectile(projectile)
  , mProjectileDamage(projectileDamage)
  , mUnknownA03E450C(unknownA03E450C)
  , mMorphballPursuitDistance(morphballPursuitDistance)
  , mIngSpotDamage(ingSpotDamage)
  , mBombStunDuration(bombStunDuration)
  , mIngSpotUnknown7569(ingSpotUnknown7569)
  , mIngSpotSfxFallOff(ingSpotSfxFallOff)
  , mIngSpot(ingSpot)
  , mAttackAngleLimit(attackAngleLimit)
  , mLineOfSightHeightOffset(lineOfSightHeightOffset)
  , mUnknownD6C8EAC2(unknownD6C8EAC2)
  , mUnknown2A5449BA(unknown2A5449BA)
  , mHearingRadius(hearingRadius)
  , mSpeedModifier(speedModifier)
  , mIngSpotAllowLockOn(ingSpotAllowLockOn)
  , mFlags155(flags155)
  , mFlags156(flags156) {}

  CAssetId mProjectile;      // +0x00: the pair the actor's CProjectileInfo is built from
  CDamageInfo mProjectileDamage; // +0x04 (0x1c) from SLdrMinorIng.projectileDamage
  float mUnknownA03E450C;    // +0x20 (SLdrMinorIng.unknown_0xa03e450c)
  float mMorphballPursuitDistance; // +0x24 (SLdrMinorIngIngSpot.morphballPursuitDistance)
  CDamageInfo mIngSpotDamage;      // +0x28 (0x1c) from SLdrMinorIngIngSpot.damage
  float mBombStunDuration;         // +0x44 (SLdrMinorIngIngSpot.bombStunDuration)
  float mIngSpotUnknown7569;       // +0x48 (SLdrMinorIngIngSpot.unknown_0x7569fdba)
  float mIngSpotSfxFallOff;        // +0x4c (SLdrMinorIngIngSpot.sfxFallOff)
  CIngSpotData mIngSpot; // +0x50 (0x60): fn_44_D54 copies exactly the shape CIngSpotData has
  float mAttackAngleLimit;         // +0x13c (SLdrMinorIng.attackAngleLimit)
  float mLineOfSightHeightOffset;  // +0x140 (SLdrMinorIng.lineOfSightHeightOffset)
  float mUnknownD6C8EAC2;          // +0x144 (SLdrMinorIng.unknown_0xd6c8eac2)
  float mUnknown2A5449BA;          // +0x148 (SLdrMinorIng.unknown_0x2a5449ba)
  float mHearingRadius;            // +0x14c (SLdrMinorIng.hearingRadius)
  float mSpeedModifier;            // +0x150 (SLdrMinorIngIngSpot.speedModifier)
  // The seven bool properties land in three guest bytes. The constructor's argument order (which
  // is what decides the pairing) does not follow the ids in SLdrMinorIng's declaration order, so
  // which property is which bit is not established: held as three opaque values until a milestone
  // reads them.
  bool mIngSpotAllowLockOn; // +0x154 bit 7 (comes from the unknown_0x3da35851 block)
  uchar mFlags155;          // +0x155
  uchar mFlags156;          // +0x156 bits 2..7
};
#endif

// The REL's CMinorIng (module 44). It derives from CPatterned directly, not from CIng [proven:
// its ctor fn_44_9D58 calls the CPatterned constructor with EPatternedAI 0x4b].
class REL_EXPORT CMinorIng : public CPatterned {
public:
#ifdef TARGET_PC
  CMinorIng(TUniqueId uid, const rstl::string& name, const CEntityInfo& info,
            const CTransform4f& xf, const CModelData& modelData, const CPatternedInfo& pInfo,
            const CActorParameters& actParms, const CMinorIngData& data);
  ~CMinorIng() override;

  // CPatterned
  using CPatterned::GetSearchPath;
  CProjectileInfo* ProjectileInfo() override { return &mProjectileInfo; }
  CPathFindSearch* GetSearchPath() override { return &mPathFindSearch; }
  CPathFindPointSearch* GetPointSearchPath() override { return &mPointPathFindSearch; }

  void PreThink(float dt, CStateManager& mgr) override;
  void Think(float dt, CStateManager& mgr) override;
  void AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) override;
  void PreRender(CStateManager& mgr) override;
  void Render(const CStateManager& mgr) const override;
  void Touch(CActor& actor, CStateManager& mgr) override;
  void Death(CStateManager& mgr, const CVector3f& direction, EScriptObjectState state) override;
  void KnockBack(CStateManager& mgr, const CKnockBackInfo& info) override;
  void SetupStateMachine(CStateManager& mgr) override;

  // CEntity
  CEntity* TypesMatch(int typeId) const override;
#endif

private:
#ifdef TARGET_PC
  void EnsureBodyActive(CStateManager& mgr);
  // Host layout (retail's CMinorIng is 0xd40, CHECK_SIZEOF below; CPatterned is 0x7c0). Guest
  // offsets are the REL's, from the constructor fn_44_9D58 and the two accessors fn_44_0
  // (this+0x960 = GetSearchPath) and fn_44_8 (this+0xa4c = GetPointSearchPath).
  CMinorIngData mData;                        // 0x7c0 (0x158)
  CProjectileInfo mProjectileInfo;            // 0x918 (0x28)
  CPathFindSearch mPathFindSearch;            // 0x960 (0xec)
  CPathFindPointSearch mPointPathFindSearch;  // 0xa4c (0x188)
  CSurfaceAlignmentHelper mSurfaceAlignment;  // 0xbd8 (0x58)
  CIngSpotPathFindNavigation mIngSpotPathFindNavigation; // 0xc30 (0x10)
  // Guest members this milestone does not hold: 0xc40 zero CVector3f, 0xc4c two locked tokens
  // (0xc64/0xc80 flags), 0xc88 CLineOfSightTracker, 0xcc8 int -1, 0xccc..0xcde ten invalid
  // TUniqueIds, 0xce0 uint, 0xcec zero and 0xcf8 one CVector3f, 0xd04..0xd38 floats,
  // 0xd3c uchar 0xff and the bitfield at 0xd3d. An empty state machine keeps none of them.
#else
  uchar x7c0_[0x158]; // CMinorIngData
  uchar x918_[0x48];  // CProjectileInfo and the three locked tokens
  uchar x960_[0x3e0]; // path find searches, surface alignment, nav, line of sight, flags
#endif
};
CHECK_SIZEOF(CMinorIng, 0xd40)

#endif
