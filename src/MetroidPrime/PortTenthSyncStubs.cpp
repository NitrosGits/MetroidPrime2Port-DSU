/**
 * Tenth-sync port stubs.
 *
 * The tenth upstream sync (`PrimeDecomp/echoes` e2a16034) added units to the port build whose
 * callers reference 14 symbols that nothing in the port build defines. The real definitions
 * exist in sources that `files.cmake` excludes (they open more undefined than they close, per
 * the ninth sync's rule), so these stand-ins close the gap. Each is an empty body, not
 * decompilation, and each is named here so a lane can replace it with the real body.
 */

#include "Kyoto/Particles/CElementGen.hpp"
#include "Kyoto/Particles/CParticleGen.hpp"
#include "MetroidPrime/CEchoEmitter.hpp"
#include "MetroidPrime/CEffect.hpp"
#include "MetroidPrime/CExplosion.hpp"
#include "MetroidPrime/Player/CPlayerKnockBackMgr.hpp"

void CPlayerKnockBackMgr::DouseFlames() {}

CParticleGen* CElementGen::ConstructChildParticleSystem(
    const CToken& description, uint type, short seed, EOptionalSystemFlags flags,
    bool modelsUseLights, bool emission, const CVector3f& translation,
    const CTransform4f& orientation, const CVector3f& globalTranslation,
    const CTransform4f& globalOrientation, const CVector3f& globalScale,
    const CColor& modulationColor, const CVector3f& localScale) {
  (void)description;
  (void)type;
  (void)seed;
  (void)flags;
  (void)modelsUseLights;
  (void)emission;
  (void)translation;
  (void)orientation;
  (void)globalTranslation;
  (void)globalOrientation;
  (void)globalScale;
  (void)modulationColor;
  (void)localScale;
  return nullptr;
}

void CElementGen::EndLifetime() {}
int CElementGen::GetParticleCountAll() const { return 0; }

void CElementGen::RenderParticlesFlameThrower(CElementGen* const* gens, int count,
                                              const CVector3f* globalTranslation,
                                              const CTransform4f* globalOrientation,
                                              const CVector3f* globalScale,
                                              const CTransform4f* globalTransform) {
  (void)gens;
  (void)count;
  (void)globalTranslation;
  (void)globalOrientation;
  (void)globalScale;
  (void)globalTransform;
}

void CParticleGen::SetGeneratorRate(float rate) { (void)rate; }
void CParticleGen::SetDrawFlags(uint flags) { (void)flags; }
float CParticleGen::GetGeneratorRate() const { return 0.f; }
uint CParticleGen::GetDrawFlags() const { return 0; }
bool CParticleGen::ShouldDraw() const { return false; }
void CParticleGen::AddModifier(CWarp* mod) { (void)mod; }

void CEchoEmitter::CreateEmitter(CStateManager& mgr) { (void)mgr; }
void CEchoEmitter::RenderEmitters(const CStateManager& mgr, const CEchoEmitter* first) {
  (void)mgr;
  (void)first;
}

CExplosion::CExplosion(const TLockedToken< CGenDescription >& particle, TUniqueId uid,
                       const CEntityInfo& info, const rstl::string& name,
                       const CTransform4f& xf, uint flags, const CVector3f& scale,
                       const CColor& color, int playerIndex)
: CEffect(uid, info, name, xf) {
  (void)particle;
  (void)flags;
  (void)scale;
  (void)color;
  (void)playerIndex;
}

// MWCC-mangled stand-ins for symbols the port references by their retail spelling
// (via `extern "C"` declarations) but nothing in the port build defines. Empty bodies;
// each names its real home for a lane to implement.
extern "C" void mwcc_SetAnimationChangeDisabled() asm("SetAnimationChangeDisabled__Q221CPlayerBodyController16SLocomotionStateFb");
extern "C" void mwcc_SetAnimationChangeDisabled() {}
extern "C" void mwcc_RenderEmitters() asm("RenderEmitters__12CEchoEmitterFRC13CStateManagerPC12CEchoEmitter");
extern "C" void mwcc_RenderEmitters() {}
extern "C" void mwcc_SendMsgs() asm("SendMsgs__14CScriptMailboxFRC7TAreaIdR13CStateManager");
extern "C" void mwcc_SendMsgs() {}
extern "C" void mwcc_GetPriority() asm("GetPriority__10CAuxEffectCFv");
extern "C" void mwcc_GetPriority() {}
