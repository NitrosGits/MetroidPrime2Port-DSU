// Port-only: the DigitalGuardian REL's echo-target actor (module 14, docs/module-analysis/
// DigitalGuardian.md "Head module"). The retail class is 0x790 bytes; its vtable lbl_14_data_10D0
// (.data 0x10D0, 31 words) keeps CActor's entries except the destructor (fn_14_1A5F4), Think
// (fn_14_1A1F8), AcceptScriptMsg (fn_14_1A430), Render (fn_14_1A168), HealthInfo (fn_14_1ABD8),
// GetHealthInfo (fn_14_10C, the base's inline copy), GetDamageVulnerability() (fn_14_1A12C),
// GetTouchBounds (fn_14_1AB50), GetOrbitPosition (fn_14_1AB98) and GetAimPosition (fn_14_19FD0).
// The comments cite the REL's section offsets.

#include "MetroidPrime/Enemies/CDigitalGuardianEchoTarget.hpp"

#include "Kyoto/Audio/CSfxManager.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/Graphics/CColor.hpp"
#include "Kyoto/Graphics/CModelFlags.hpp"
#include "Kyoto/Particles/CElementGen.hpp"
#include "MetroidPrime/CActorParameters.hpp"
#include "MetroidPrime/CEchoEmitter.hpp"
#include "MetroidPrime/CEntityInfo.hpp"
#include "MetroidPrime/CModelData.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"
#include "MetroidPrime/ScriptLoader.hpp"

namespace {

// lbl_14_bss_34 and lbl_14_bss_38: the static initialiser fn_14_1AD2C builds the hit colour from
// .rodata 0x9A0 (0.5), 0x960 (0) and 0x96C (1), and the box half-extent from 0x968 (2).
const CColor sEchoTargetHitColor(0.5f, 0.f, 0.f, 1.f); // lbl_14_bss_34
const CVector3f sEchoTargetHalfExtent(2.f, 2.f, 2.f);   // lbl_14_bss_38

// The box round a point: min is the point less the half-extent, max the point plus it.
CAABox EchoTargetBox(const CVector3f& pos) {
  return CAABox(pos - sEchoTargetHalfExtent, pos + sEchoTargetHalfExtent);
}

// The retail's PassThruVulnerability is the host's CDamageVulnerability::PassThroughVulnerabilty;
// the retail's name is kept so the asm-vs-C++ check can match the call.
const CDamageVulnerability& PassThruVulnerability() {
  return CDamageVulnerability::PassThroughVulnerabilty();
}

} // namespace

// @ fn_14_1A698
CDigitalGuardianEchoTarget::CDigitalGuardianEchoTarget(
    TUniqueId uid, const rstl::string& name, const CEntityInfo& info, uint inGrave,
    const CTransform4f& xf, const CModelData& mData, const CMaterialList& list,
    const CDamageVulnerability& vuln, TUniqueId owner, const SLdrDigitalGuardianHeadData& data)
: CActor(uid, name, info, inGrave, xf, mData, list, CActorParameters(), owner)
, x158_(data) // fn_14_1912C
, x708_(kS_Retracted)
, x70C_(0.f) // .rodata 0x960
, x710_(0.f)
, x714_(owner)
, x718_(EchoTargetBox(xf.GetTranslation())) // 0x1A6F4..0x1A770, the flag at 0x1A774
, x734_(vuln)                               // fn_14_3B4C
, x764_(data.echoTargetHP, 10.f)            // data +0x98, .rodata 0x980
, x784_(nullptr)
, x788_()
, x78C_24_(false) // the rlwimi at 0x1A7B8
{
  SetVisorOrbitableFlags(CVisorParameters::kVOF_All, false); // li r4, 0xF
  SetVisorOrbitableFlags(CVisorParameters::kVOF_Dark, true); // li r4, 0x8
  // The emitter effect (data +0xA0, 0x1A7D8..0x1A8B4): a 'PART' token, kMOT_Normal, kOSF_One.
  if (data.echoTargetEmitterFx != kInvalidAssetId) {
    TToken< CGenDescription > token(gpSimplePool->GetObj(SObjectTag('PART', data.echoTargetEmitterFx)));
    CElementGen* gen = rs_new CElementGen(token, CElementGen::kMOT_Normal, CElementGen::kOSF_One);
    delete x784_;
    x784_ = gen;
  }
  // The echo emitter (0x1A8B8..0x1A98C): the box 5 units up the up column (.rodata 0x97C) with
  // the echo parameters (data +0x120).
  const CVector3f centre = GetTranslation() + GetTransform().GetColumn(kDZ) * 5.f;
  AllocateEchoEmitter(true, EchoTargetBox(centre),
                      LdrToEchoParameters(x158_.echoTargetsParameters));
}

// @ fn_14_1A5F4
// The emitter effect goes through its virtual destructor; the vulnerability, the parameter
// block and the CActor base are the compiler's.
CDigitalGuardianEchoTarget::~CDigitalGuardianEchoTarget() { delete x784_; }

// fn_14_1A1F8: only while active (the extrwi at 0x1A21C).
void CDigitalGuardianEchoTarget::Think(float dt, CStateManager& mgr) {
  if (!GetActive()) {
    return;
  }
  CActor::Think(dt, mgr);

  // The hit flash (0x1A230..0x1A2B8): the timer counts down and lerps the colour.
  if (x70C_ > 0.f) {
    float left = x70C_ - dt;
    if (!(left > 0.f)) {
      left = 0.f;
    }
    x70C_ = left;
    SetModelFlags(CModelFlags(CModelFlags::kT_Two, 0, static_cast< CModelFlags::EFlags >(3),
                              CColor::Lerp(CColor::Black(), sEchoTargetHitColor, x70C_)));
  }
  x19CE0_(mgr, dt);
  x19B70_(mgr, dt);
  x19A08_(mgr);

  // The touch bounds: the box 5 units up the up column (0x1A2C8..0x1A3C4).
  const CVector3f centre = GetTranslation() + GetTransform().GetColumn(kDZ) * 5.f;
  x718_ = EchoTargetBox(centre);

  // The echo emitter's bounds follow the box (0x1A3C8..0x1A41C).
  if (EchoEmitter() != nullptr) {
    EchoEmitter()->SetBounds(x718_.data());
  }
}

// fn_14_1A430: only "XDMG" (0x1A458), and not while ignored (0x78C bit 7, the extrwi at
// 0x1A470). The flash restarts (.rodata 0x96C, 1); at no HP (the HealthInfo's HP at 0x768,
// fcmpo/cror le at 0x1A48C) the target is destroyed: sound_EchoTargetExplosion (data +0x108) and
// the transmit sound stops; otherwise sound_EchoTargetHit (data +0xF0). The owner gets the XDMG
// with this target as the sender, and the echo emitter the flash time.
void CDigitalGuardianEchoTarget::AcceptScriptMsg(CStateManager& mgr, const CScriptMsg& msg) {
  const EScriptObjectMessage message = msg.GetMessage();
  CActor::AcceptScriptMsg(mgr, msg);
  if (message != kSM_Damage) {
    return;
  }
  if (x78C_24_) {
    return;
  }
  x70C_ = 1.f;
  if (x764_.GetHP() <= 0.f) {
    x708_ = kS_Destroyed;
    PlayCustomSound(GetAimPosition(mgr, 0.f), GetTransform().GetColumn(kDZ),
                    x158_.sound_EchoTargetExplosion, false);
    if (x788_ != CSfxHandle()) {
      CSfxManager::RemoveEmitter(x788_);
      x788_ = CSfxHandle();
    }
  } else {
    PlayCustomSound(GetAimPosition(mgr, 0.f), GetTransform().GetColumn(kDZ),
                    x158_.sound_EchoTargetHit, false);
  }
  mgr.DeliverScriptMsg(CScriptMsg(GetUniqueId(), x714_, kSM_Damage)); // 0x1A578..0x1A5B4
  if (EchoEmitter() != nullptr) { // the retail's test at 0x1A5C0
    EchoEmitter()->SetAutoReducingDamage(x70C_);
  }
}

// Extending or extended: the emitter effect, then the model; retracting: the model only;
// retracted or destroyed: nothing.
void CDigitalGuardianEchoTarget::Render(const CStateManager& mgr) const {
  switch (x708_) {
  case kS_Extended:
  case kS_Extending:
    if (x784_ != nullptr) {
      x784_->Render();
    }
    CActor::Render(mgr);
    break;
  case kS_Retracting:
    CActor::Render(mgr);
    break;
  default:
    break;
  }
}

// @ fn_14_1A12C
// Extended or extending: the echo-target vulnerability (0x734); otherwise pass-through.
const CDamageVulnerability* CDigitalGuardianEchoTarget::GetDamageVulnerability() const {
  if (x708_ == kS_Extended || x708_ == kS_Extending) {
    return &x734_;
  }
  return &PassThruVulnerability();
}

rstl::optional_object< CAABox > CDigitalGuardianEchoTarget::GetTouchBounds() const {
  return x718_;
}

CVector3f CDigitalGuardianEchoTarget::GetOrbitPosition(const CStateManager& mgr) const {
  return GetAimPosition(mgr, 0.f); // .rodata 0x960
}

// fn_14_19FD0: the box's centre, led by the owner's motion over dt (its aim position at dt less
// its aim position now).
// @ fn_14_19FD0
CVector3f CDigitalGuardianEchoTarget::GetAimPosition(const CStateManager& mgr, float dt) const {
  CVector3f pos = x718_.data().GetCenterPoint();
  const CActor* owner = static_cast< const CActor* >(mgr.GetObjectById(x714_));
  if (owner != nullptr) {
    const CVector3f now = owner->GetAimPosition(mgr, 0.f);
    const CVector3f later = owner->GetAimPosition(mgr, dt);
    pos = (later + pos) - now;
  }
  return pos;
}

// fn_14_19DF8: never out of the destroyed state; retracting plays sound_EchoTargetRetract (data
// +0xC0), extending sound_EchoTargetExtract (data +0xA8), both one-shot.
void CDigitalGuardianEchoTarget::SetState(CStateManager& mgr, int state) {
  if (x708_ == kS_Destroyed || x708_ == state) {
    return;
  }
  x708_ = state;
  switch (x708_) {
  case kS_Retracting:
    PlayCustomSound(GetAimPosition(mgr, 0.f), GetTransform().GetColumn(kDZ),
                    x158_.sound_EchoTargetRetract, false);
    break;
  case kS_Extending:
    PlayCustomSound(GetAimPosition(mgr, 0.f), GetTransform().GetColumn(kDZ),
                    x158_.sound_EchoTargetExtract, false);
    break;
  default:
    break;
  }
}

// @ fn_14_19F18
// The translation moves 6 units (.rodata 0x990) times the extension along xf's up column.
void CDigitalGuardianEchoTarget::SetExtendedTransform(const CTransform4f& xf) {
  const CVector3f offset = xf.GetColumn(kDZ) * (6.f * x710_);
  CTransform4f moved(xf);
  moved.SetTranslation(moved.GetTranslation() + offset);
  SetTransform(moved);
}

// fn_14_19A08: extending, the looped transmit sound (data +0xD8) at the aim position along the up
// column, full volume (0x7F) in the echo visor and the sound's max volume (the low byte at data
// +0xE4) otherwise; any other state stops it.
void CDigitalGuardianEchoTarget::x19A08_(CStateManager& mgr) {
  switch (x708_) {
  case kS_Extending: {
    const CVector3f pos = GetAimPosition(mgr, 0.f);
    if (x788_ == CSfxHandle()) {
      x788_ = PlayCustomSound(pos, GetTransform().GetColumn(kDZ),
                              x158_.sound_EchoTargetTransmit, true);
      return;
    }
    const uchar vol = mgr.GetPlayerState()->GetActiveVisor(mgr) == CPlayerState::kPV_Echo
                          ? 0x7F
                          : static_cast< uchar >(x158_.sound_EchoTargetTransmit.maxVolume);
    CSfxManager::UpdateEmitter(x788_, pos, GetTransform().GetColumn(kDZ), vol);
    return;
  }
  default:
    if (x788_ != CSfxHandle()) {
      CSfxManager::RemoveEmitter(x788_);
      x788_ = CSfxHandle();
    }
    return;
  }
}

// fn_14_19B70: extending or extended, the effect emits, faces up the up column from the aim
// position and steps; otherwise it stops emitting.
void CDigitalGuardianEchoTarget::x19B70_(CStateManager& mgr, float dt) {
  if (x784_ == nullptr) {
    return;
  }
  if (x708_ == kS_Extending || x708_ == kS_Extended) {
    x784_->SetParticleEmission(true);
    const CVector3f pos = GetAimPosition(mgr, 0.f);
    const CVector3f up = GetTransform().GetColumn(kDZ);
    const CTransform4f xf = CTransform4f::LookAt(pos, pos + up, CVector3f::Up());
    x784_->SetGlobalOrientation(xf);
    x784_->SetGlobalTranslation(pos);
    x784_->Update(dt);
    return;
  }
  x784_->SetParticleEmission(false);
}

// fn_14_19CE0: retracting, the extension falls 0.75/s (.rodata 0x9FC) to 0 and the target is
// retracted; extending, it rises 0.25/s (.rodata 0x99C) to 1 and the target is extended. Extended
// and extending targets carry the orbit, target and seeker-target materials (41, 40, 63).
void CDigitalGuardianEchoTarget::x19CE0_(CStateManager& mgr, float dt) {
  switch (x708_) {
  case kS_Retracting:
    x710_ = x710_ - 0.75f * dt;
    if (x710_ <= 0.f) {
      x710_ = 0.f;
      x708_ = kS_Retracted;
    }
    RemoveMaterial(kMT_Orbit, kMT_Target, kMT_SeekerTarget, mgr);
    break;
  case kS_Extending:
    x710_ += 0.25f * dt;
    if (x710_ >= 1.f) {
      x710_ = 1.f;
      x708_ = kS_Extended;
    }
    AddMaterial(kMT_Orbit, kMT_Target, kMT_SeekerTarget, mgr);
    break;
  case kS_Extended:
    AddMaterial(kMT_Orbit, kMT_Target, kMT_SeekerTarget, mgr);
    break;
  case kS_Destroyed:
  case kS_Retracted:
    RemoveMaterial(kMT_Orbit, kMT_Target, kMT_SeekerTarget, mgr);
    break;
  default:
    break;
  }
}
