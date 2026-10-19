// Ingsmasher (CElitePirate, REL 15): the FSM combat states (shield, shock wave, melee, rockets, angry attack, taunt, power up/down, death).
// Part of the module body in PortCElitePirate.cpp; analysis in docs/module-analysis/ElitePirate.md.
#include "MetroidPrime/Enemies/CElitePirate.hpp"
#include "Collision/CMaterialFilter.hpp"
#include "Kyoto/Animation/CPASAnimParmData.hpp"
#include "Kyoto/Animation/CSkinnedModel.hpp"
#include "Kyoto/CRandom16.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Math/CPlane.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetaRender/CCubeRenderer.hpp"
#include "MetroidPrime/BodyState/CBodyController.hpp"
#include "MetroidPrime/BodyState/CBodyStateCmdMgr.hpp"
#include "MetroidPrime/CAnimData.hpp"
#include "MetroidPrime/CAxisAngle.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/CCollisionActor.hpp"
#include "MetroidPrime/CCollisionActorManager.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/CGameArea.hpp"
#include "MetroidPrime/CGameCollision.hpp"
#include "MetroidPrime/CHealthInfo.hpp"
#include "MetroidPrime/CRumbleManager.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CWorld.hpp"
#include "MetroidPrime/Cameras/CFirstPersonCamera.hpp"
#include "MetroidPrime/Collision/CJointCollisionDescription.hpp"
#include "MetroidPrime/Enemies/CElitePirateGrenadeLauncher.hpp"
#include "MetroidPrime/Enemies/CPatternedInfo.hpp"
#include "MetroidPrime/PathFinding/CPathFindArea.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/ScriptLoader.hpp"
#include "MetroidPrime/ScriptLoader/SLdrElitePirate.hpp"
#include "MetroidPrime/ScriptLoaderRel.hpp"
#include "MetroidPrime/ScriptObjects/CScriptAIHint.hpp"
#include "MetroidPrime/ScriptObjects/CScriptTeamAiMgr.hpp"
#include "MetroidPrime/TCastTo.hpp"
#include "MetroidPrime/Weapons/CEnergyProjectile.hpp"
#include "MetroidPrime/Weapons/CGameProjectile.hpp"
#include "MetroidPrime/Weapons/CImpactVisorEffect.hpp"
#include "MetroidPrime/Weapons/WeaponTypes.hpp"
#include <float.h>
#include <math.h>

void ElitePirateStandIn(const char* what); // PortCElitePirate.cpp

void CElitePirate::Shielding(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("Shielding (fn_15_41A0)");
}

void CElitePirate::ShieldUp(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("ShieldUp (fn_15_44A8)");
}

void CElitePirate::Shockwave(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("Shockwave (fn_15_4AE0)");
}

void CElitePirate::MeleeAttack(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("MeleeAttack (fn_15_48BC)");
}

void CElitePirate::ProjectileAttack(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("ProjectileAttack (fn_15_50BC)");
}

void CElitePirate::SpreadShot(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("SpreadShot (fn_15_4D18)");
}

void CElitePirate::AngryAttackBegin(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("AngryAttackBegin (fn_15_1900)");
}

void CElitePirate::AngryAttackEnd(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("AngryAttackEnd (fn_15_18D8)");
}

void CElitePirate::Taunt(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("Taunt (fn_15_5A60)");
}

void CElitePirate::PowerUp(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("PowerUp (fn_15_5F08)");
}

void CElitePirate::PowerDown(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("PowerDown (fn_15_5FF0)");
}

void CElitePirate::Dead(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("Dead (fn_15_7B00)");
}
