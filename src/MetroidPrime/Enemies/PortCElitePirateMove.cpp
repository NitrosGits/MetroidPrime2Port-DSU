// Ingsmasher (CElitePirate, REL 15): the FSM movement states (patrol, pursue, turn, alert, wait, stun, attack pattern).
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

void CElitePirate::Patrol(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("Patrol (fn_15_5708)");
}

void CElitePirate::TargetPatrol(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("TargetPatrol (fn_15_578C)");
}

void CElitePirate::Pursue(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("Pursue (fn_15_5BB4)");
}

void CElitePirate::Turn(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("Turn (fn_15_5598)");
}

void CElitePirate::Alert(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("Alert (fn_15_5340)");
}

void CElitePirate::InvulnAlert(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("InvulnAlert (fn_15_52D0)");
}

void CElitePirate::Wait(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("Wait (fn_15_6064)");
}

void CElitePirate::Stunned(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("Stunned (fn_15_59BC)");
}

void CElitePirate::FollowAttackPattern(CStateManager&, EStateMsg, float) {
  ElitePirateStandIn("FollowAttackPattern (fn_15_6DDC)");
}
