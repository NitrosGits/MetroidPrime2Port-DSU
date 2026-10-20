#ifndef _PORTBOOSTGUARDIANSHARED
#define _PORTBOOSTGUARDIANSHARED

// Port-only: the pieces the Boost Guardian's state machine shares between its files - the form
// constants, the property-flag bits, the joint-sphere table and the small helpers the constructor
// and the state bodies use. The behaviour comes from the Ing host body (branch decomp/ing,
// src/MetroidPrime/Enemies/PortCIng.cpp); this module (the Ing REL with the boost half added)
// carries its own copy of the Ing code, so the names are the Ing REL's.

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/Animation/CSegId.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Particles/CGenDescription.hpp"
#include "Kyoto/SObjectTag.hpp"
#include "Kyoto/TToken.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Collision/CMaterialList.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Enemies/CIngBoostBallGuardian.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/PathFinding/CPathFindPointSearch.hpp"

#include <stdlib.h>

// The forms (mForm, this module's guest 0xAE4). 0 is the host form SuckEnergy sets (the Ing
// possesses the player and has no body of its own), 1 is the exit host form ExitHost's Activate
// sets, 2 corporeal, 3 ing spot, 4 the flying body projectile, 5 evaporating; 6/7/8/9 are the four
// form changes' own forms.
enum {
  kForm_Host = 0,
  kForm_ControllingHost = 1, // the second host form, ExitHost's Activate; nothing else reads it
  kForm_Corporeal = 2,
  kForm_IngSpot = 3,
  kForm_BodyProjectile = 4,
  kForm_Evaporate = 5,
  kForm_BecomingCorporeal = 6, // BecomeCorporeal's Activate, when it leaves the ing-spot form
  kForm_BecomingIngSpot = 7,   // BecomeIngSpot's Activate, when it leaves the corporeal form
  kForm_BecomingBodyProjectile = 8, // BecomeBodyProjectile's Activate
  kForm_BecomingWallProjectile = 9, // BecomeWallProjectile's Activate
};

// mData.mFlags210: the parameter block's flag byte (guest +0x320 of the block, this+0xAE0). It is
// not the property's raw value: the loader fn_30_10C24 derives every bit from the property
// SLdrIngBoostBallGuardianData.unknown_0x01789abd (r10 = its word, 0x00010D24) - see
// PackBgDataFlags. Each name comes from the bodies that test the bit (asm offsets of the reads).
enum {
  // 0x01: never written by the loader and never read.
  kFlag_AlwaysTargetable = 0x02, // fn_30_5CD0 0x00005D58: the ing spot stays targetable
  kFlag_NoPathFind = 0x04,       // fn_30_9F50 0x00009FFC/0x0000A044, fn_30_9DA0 0x00009DFC,
                                 // fn_30_9C7C 0x00009CC8
  kFlag_NoFormChange = 0x08,     // fn_30_C070 0x0000C0A8/0x0000C0D0, fn_30_BD48 0x0000BD70
  kFlag_NoMiniPortal = 0x10,     // fn_30_BC2C 0x0000BC4C
  kFlag_NoBodyProjectile = 0x20, // fn_30_BD48 0x0000BD68, fn_30_8520 0x00008700
  kFlag_NoArmSwipe = 0x40,       // fn_30_BE94 0x0000BEC0, fn_30_A0D8 0x0000A174
  kFlag_StartAsIngSpot = 0x80,   // fn_30_68F0 0x00006914/0x00006928, ctor 0x0000FE60
};

// fn_30_10C24 0x00010D20-0x00010DE8: the flag byte, built bit by bit with rlwimi from the
// property word r10: r6 = neg|or of (r10 & 3) (0x00010D30, 0x00010D3C-0x00010D40) and r5 = neg|or
// of (r10 & 5) (0x00010D34, 0x00010D4C-0x00010D50) carry "non-zero" in their sign bit, r4 =
// (r10 & 6) != 0 (rlwinm 0,29,30 at 0x00010D44, neg/or/srwi 31 at 0x00010D58-0x00010D64), and
// r11 = 0 (0x00010D20). Bit 0x01 is not written (and never read).
inline uint PackBgDataFlags(uint raw) {
  uint flags = 0;
  // 0x00010D9C rlwimi 7,24,24 from r11 = 0: kFlag_StartAsIngSpot is always clear.
  if ((raw & 0x1) != 0) { // 0x00010DA8 rlwimi r10,6,25,25: bit 0 of the word
    flags |= kFlag_NoArmSwipe;
  }
  if ((raw & 0x2) != 0) { // 0x00010DB4 rlwimi r10,4,26,26: bit 1 of the word
    flags |= kFlag_NoBodyProjectile;
  }
  if ((raw & 0x3) != 0) { // 0x00010DC0 rlwimi r6,5,27,27: (word & 3) != 0
    flags |= kFlag_NoMiniPortal;
  }
  if ((raw & 0x4) != 0) { // 0x00010DCC rlwimi r10,1,28,28: bit 2 of the word
    flags |= kFlag_NoFormChange;
  }
  if ((raw & 0x5) != 0) { // 0x00010DD8 rlwimi r5,3,29,29: (word & 5) != 0
    flags |= kFlag_NoPathFind;
  }
  if ((raw & 0x6) != 0) { // 0x00010DE4 rlwimi r4,1,30,30: (word & 6) != 0
    flags |= kFlag_AlwaysTargetable;
  }
  return flags;
}

const float kFacingAngle = 0.349f; // rodata: 20 degrees
// rodata_104: the wall projectile's launch direction is clamped to this angle from the surface
// normal.
const float kWallLaunchAngle = 0.7853981f; // 45 degrees

// The seven joint spheres: the Ing REL's own rodata_0 table, seven (locator, radius) pairs. Every
// radius is 1.5 (0x3FC00000); the pivot point is the zero vector and the mass is 1000.0 (rodata_C4).
// The names are the Ing model's locators.
struct SIngJointSphere {
  const char* mLocator;
  float mRadius;
};
const SIngJointSphere skIngJointSpheres[] = {
    {"head", 1.5f},   {"Pelvis_SDK", 1.5f}, {"L_knee", 1.5f}, {"R_knee", 1.5f},
    {"F_elbow", 1.5f}, {"L_elbow", 1.5f},   {"R_elbow", 1.5f},
};
const float kIngJointMass = 1000.f; // rodata_C4

// Reports a stand-in the first time it runs (PortCIngBoostBallGuardian.cpp).
void BgStandIn(const char* what);

// MP2_DEBUG_BG turns on the state machine's trace.
inline bool DebugBg() {
  static int sEnabled = -1;
  if (sEnabled < 0) {
    const char* env = getenv("MP2_DEBUG_BG");
    sEnabled = env != nullptr && env[0] != '\0' && env[0] != '0' ? 1 : 0;
  }
  return sEnabled != 0;
}

// Per-trigger debug throttle: one line in sixty (about a second), keyed by the caller's own
// counter. The body-projectile triggers are evaluated every frame while their state is active, so
// a raw print would bury the log.
inline bool BgLogTick(int& counter) {
  return DebugBg() && (counter++ % 60) == 0;
}

// The Ing's optional GEN token for a GEN asset id.
inline rstl::optional_object< TLockedToken< CGenDescription > > BgOptionalPart(CAssetId id) {
  if (id == kInvalidAssetId) {
    return rstl::optional_object< TLockedToken< CGenDescription > >();
  }
  return rstl::optional_object< TLockedToken< CGenDescription > >(
      TLockedToken< CGenDescription >(gpSimplePool->GetObj(SObjectTag('PART', id))));
}

// The ctor's "head" locator (the ctor's first GetLocatorSegId). It is the line-of-sight tracker's
// segment and BecomeCorporeal's Activate restores it.
inline CSegId BgHeadSegId(const CModelData& model) {
  const CAnimData* anim = model.GetAnimationData();
  return anim != nullptr ? anim->GetLocatorSegId(rstl::string_l("head")) : CSegId();
}

// The ctor's arm locators, same lookup as the head's.
inline CSegId BgArmSegId(const CModelData& model, const char* name) {
  const CAnimData* anim = model.GetAnimationData();
  return anim != nullptr ? anim->GetLocatorSegId(rstl::string_l(name)) : CSegId();
}

// Point search between two points of the area's point graph (retail indexes CPFArea's point array,
// +0x190, with stride 0x1c). The Ing REL's own helper (fn_29_1C98's callers use it).
inline bool BgPointSearch(CPathFindPointSearch& search, CPFArea& area, int from, int to) {
  return search.Search(area.GetPoint(from), area.GetPoint(to)) == CPathFindPointSearch::kR_Success;
}

// The Ing host body's SolidFilter(): one included material, kMT_Solid, and an empty exclude list.
inline CMaterialFilter SolidFilter() {
  return CMaterialFilter::MakeIncludeExclude(CMaterialList(kMT_Solid), CMaterialList());
}

#endif // _PORTBOOSTGUARDIANSHARED
