#include "port_radial.h"
#include "port_entry.h"
#include "port_harness.h"
#include "port_mouse.h"
#include "port_overlay.h"
#include "port_photo.h"

#include "GuiSys/CGuiFrame.hpp"
#include "GuiSys/CGuiWidget.hpp"
#include "GuiSys/CGuiWidgetDrawParms.hpp"
#include "Kyoto/CResFactory.hpp"
#include "Kyoto/CSimplePool.hpp"
#include "Kyoto/TToken.hpp"
#include "MetroidPrime/CStateManager.hpp"
#include "MetroidPrime/CCameraManager.hpp"
#include "MetroidPrime/Player/CPlayer.hpp"
#include "MetroidPrime/Player/CPlayerGun.hpp"
#include "MetroidPrime/Player/CPlayerState.hpp"

#include <cstdio>
#include <cmath>

namespace PortRadial {
namespace {
// Echoes enum order, verified against the actual RS5 widget names/idle positions.
const char* const kNames[2][4] = {{"power", "dark", "light", "annihilator"},
                                 {"combat", "echo", "scan", "dark"}};
const char* const kFrames[] = {"FRME_BeamSelect", "FRME_VisorSelect"};
const char* const kBases[] = {"basewidget_beamSelect", "basewidget_visorSelect"};
// All declared dependencies of these specific Echoes frames (embedded models, no external CMDL).
const unsigned kTextures[2][5] = {{0x6ae2fc30, 0x1c42d5aa, 0x73d22938, 0x2c6b84d0, 0x07f78329},
                                 {0x23a06a33, 0x6ebab477, 0x84b3f2f8, 0xf51bf04c, 0x07f78329}};

struct Frame {
  TCachedToken<CGuiFrame>* token = nullptr;
  CGuiFrame* frame = nullptr;
  CGuiWidget* icons[4] = {};
  CGuiWidget* highlights[4] = {};
  Point positions[4];
  float alpha[4] = {};
  bool started = false, bound = false, failed = false;
  float waiting = 0.f;
  ~Frame() { delete token; }

  bool Fail(int kind, const char* why) {
    if (!failed) std::fprintf(stderr, "[radial] %s unavailable: %s; existing controls retained\n",
                             kFrames[kind], why);
    failed = true;
    return false;
  }
  bool Load(int kind, float dt) {
    if (failed) return false;
    if (!started) {
      const SObjectTag* tag = gpResourceFactory->GetResourceIdByName(kFrames[kind]);
      if (!tag || !gpResourceFactory->CanBuild(*tag)) return Fail(kind, "missing FRME");
      for (int i = 0; i < 5; ++i) {
        if (!gpResourceFactory->CanBuild(SObjectTag('TXTR', kTextures[kind][i])))
          return Fail(kind, "missing texture dependency");
      }
      token = new TCachedToken<CGuiFrame>(gpSimplePool->GetObj(*tag));
      token->Lock();
      started = true;
    }
    if (!token->TryCache() || !token->GetObject()->GetIsFinishedLoading()) {
      waiting += dt;
      if (waiting > 15.f) return Fail(kind, "load completeness timeout");
      return false;
    }
    frame = token->GetObject();
    if (!bound) {
      if (!frame->GetFrameCamera() || !frame->FindWidget(kBases[kind]) ||
          !frame->FindWidget("model_main_frame") || !frame->FindWidget("model_inner_ring"))
        return Fail(kind, "missing camera/base/frame/ring");
      for (int i = 0; i < 4; ++i) {
        char name[80];
        std::snprintf(name, sizeof(name), "model_%s_icon", kNames[kind][i]);
        icons[i] = frame->FindWidget(name);
        std::snprintf(name, sizeof(name), "model_highlight_%s", kNames[kind][i]);
        highlights[i] = frame->FindWidget(name);
        if (!icons[i] || !highlights[i]) return Fail(kind, "missing icon/highlight");
        const CVector3f p = icons[i]->GetIdlePosition();
        positions[i] = {p.GetX(), p.GetZ()};
        std::fprintf(stderr, "[radial] %s slot %d %s at %.5f,%.5f\n", kFrames[kind], i,
                     kNames[kind][i], p.GetX(), p.GetZ());
      }
      // CGuiCamera already widens projection without stretching; no horizontal widget scaling.
      bound = true;
      std::fprintf(stderr, "[radial] %s fully loaded and bound\n", kFrames[kind]);
    }
    return true;
  }
};
Frame* sFrames = nullptr;
}

bool GameplayAllowed(const CStateManager& mgr) {
  if (!port::entry::IsTrilogy(port::entry::CurrentDiscVersion()) ||
      !PortOverlay::TrilogyRadialSelectors() || PortOverlay::PointerBlocked() || PortPhoto::Active() ||
      PortHarness::FlowState() != 1 || mgr.IsMultiplayer() ||
      mgr.GetGameState() != CStateManager::kGS_Running) return false;
  const CPlayer* p = mgr.GetPlayer(0);
  return p && !p->IsInTurret() && p->GetPlayerState()->GetHealthInfo().GetHP() > 0.f &&
         p->GetMorphballTransitionState() == CPlayer::kMS_Unmorphed &&
         p->GetCameraManager() && p->GetCameraManager()->IsInFPCamera() &&
         !p->GetCameraManager()->IsInCinematicCamera();
}

void UpdateHud(float dt, const CStateManager& mgr, int player) {
  if (player != 0) return;
  if (!GameplayAllowed(mgr)) {
    SetEligible(false, false);
    Transport().SetAvailable(false);
    Cancel();
    return;
  }
  if (!sFrames) sFrames = new Frame[2];
  const bool beam = sFrames[0].Load(0, dt);
  const bool visor = sFrames[1].Load(1, dt);
  Transport().SetAvailable(beam && visor);
  if (!beam || !visor) return;
  const CPlayerState& ps = *mgr.GetPlayer(0)->GetPlayerState();
  const CPlayerGun* gun = mgr.GetPlayer(0)->GetPlayerGun();
  // Latch gate: gun idle; the beam wheel also not in the Scan visor.
  const bool ready = gun && gun->PortRadialReady();
  SetEligible(ready && ps.GetCurrentVisor() != CPlayerState::kPV_Scan, ready);
  SetApplied(gun ? int(gun->GetPrimaryWeaponId()) : -1, int(ps.GetCurrentVisor()));
  for (int k = 0; k < 2; ++k) {
    Frame& f = sFrames[k];
    unsigned owned = 0;
    for (int i = 0; i < 4; ++i) {
      const bool has = k == kBeam ? ps.HasPowerUp(static_cast<CPlayerState::EItemType>(i)) :
                                   ps.HasVisor(static_cast<CPlayerState::EPlayerVisor>(i)) != 0;
      if (has) owned |= 1u << i;
      f.icons[i]->SetIsVisible(has);
      f.highlights[i]->SetIsVisible(has);
    }
    if (Transport().Active() == k) {
      const int current = k == kBeam ? int(ps.GetCurrentBeam()) : int(ps.GetTransitioningVisor());
      Transport().Resolve(f.positions, owned, current);
    }
    const int selected = Transport().Active() == k ? Transport().Selection(f.positions) : -1;
    for (int i = 0; i < 4; ++i) {
      const float goal = selected == i ? 1.f : 0.f;
      const float step = dt * 4.f;
      f.alpha[i] += goal > f.alpha[i] ? (goal - f.alpha[i] < step ? goal - f.alpha[i] : step) :
                                      (f.alpha[i] - goal < step ? goal - f.alpha[i] : -step);
      f.highlights[i]->SetColor(CColor::White().WithAlphaOf(f.alpha[i]));
    }
    f.frame->Update(dt);
  }
  // Donor-inspired fade/highlight approach, not claimed as Echoes retail timing/color behavior.
  Transport().Animate(dt);
}

void DrawHud(int player) {
  if (player != 0 || !sFrames || !PortOverlay::TrilogyRadialSelectors() ||
      PortOverlay::PointerBlocked() || PortHarness::FlowState() != 1) return;
  for (int k = 0; k < 2; ++k) {
    if (sFrames[k].bound && Transport().Open(static_cast<Kind>(k)) > 0.f)
      sFrames[k].frame->Draw(CGuiWidgetDrawParms(Transport().Open(static_cast<Kind>(k))));
  }
}

void DestroyHud(int player) {
  if (player != 0) return;
  SetEligible(false, false);
  Transport().SetAvailable(false);
  SetApplied(-1, -1);
  Cancel();
  delete[] sFrames;
  sFrames = nullptr;
}
}
