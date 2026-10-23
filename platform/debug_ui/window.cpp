// F1 overlay: Touch scrolling, the page and desktop windows, and DrawUI. State and helpers shared
// with the other parts are in debug_ui_internal.h.

#include "debug_ui_internal.h"

#include "port_env.h"
#include "port_input_remap_ui.h"
#include "port_mouse.h"
#include "port_overlay.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <string>

namespace PortDebug {

namespace {

bool IsResizeGrip(ImGuiWindow* window, ImGuiID id) {
  for (int n = 0; n < 4; ++n) {
    if (id == ImGui::GetWindowResizeCornerID(window, n) ||
        id == ImGui::GetWindowResizeBorderID(window, static_cast< ImGuiDir >(n))) {
      return true;
    }
  }
  return false;
}

// ImGui has no touch scrolling: a finger dragged down a page presses whatever
// it landed on and scrolls nothing. A mostly vertical drag is taken away from
// the widget it began on (so a button under it does not fire on release) and
// scrolls the window instead, and the finger's speed carries on as a fling
// after it lifts. A mostly horizontal drag is left alone, so sliders still
// work, and so are the scrollbar, a window being moved or resized, and drags
// that start outside the content area. Runs after NewFrame, before any window.
struct TouchScroll {
  ImGuiWindow* window = nullptr;
  bool decided = false;
  bool dragging = false;
  float velocity = 0.f; // pixels per second, positive scrolls down
};
TouchScroll sTouchScroll;

void UpdateTouchScroll() {
  ImGuiContext& g = *ImGui::GetCurrentContext();
  const ImGuiIO& io = g.IO;
  TouchScroll& scroll = sTouchScroll;
  const bool touch = io.MouseSource == ImGuiMouseSource_TouchScreen;

  if (io.MouseClicked[0]) {
    scroll = TouchScroll{};
    if (touch) {
      ImGuiWindow* window = ScrollableWindowAt(g.HoveredWindow);
      if (window != nullptr && window->InnerClipRect.Contains(io.MouseClickedPos[0])) {
        scroll.window = window;
      }
    }
  }
  if (scroll.window == nullptr) {
    return;
  }

  if (io.MouseDown[0]) {
    if (!scroll.decided) {
      const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.f);
      // A share of the font size rather than io.MouseDragThreshold's fixed
      // pixels, which on a dense phone screen is a jitter, not a drag.
      const float threshold = g.FontSize * 0.6f;
      if (delta.x * delta.x + delta.y * delta.y < threshold * threshold) {
        return;
      }
      scroll.decided = true;
      ImGuiWindow* activeWindow = g.ActiveIdWindow;
      const bool ownDrag =
          g.MovingWindow != nullptr ||
          (g.ActiveId != 0 && activeWindow != nullptr &&
           (g.ActiveId == ImGui::GetWindowScrollbarID(activeWindow, ImGuiAxis_X) ||
            g.ActiveId == ImGui::GetWindowScrollbarID(activeWindow, ImGuiAxis_Y) ||
            IsResizeGrip(activeWindow, g.ActiveId)));
      if (std::fabs(delta.y) <= std::fabs(delta.x) || ownDrag) {
        scroll.window = nullptr;
        return;
      }
      scroll.dragging = true;
    }
    if (!scroll.dragging) {
      return;
    }
    if (g.ActiveId != 0) {
      ImGui::ClearActiveID();
    }
    ImGui::SetScrollY(scroll.window, scroll.window->Scroll.y - io.MouseDelta.y);
    if (io.DeltaTime > 0.f) {
      // Smoothed, so the last jittery frame before the finger lifts does not
      // decide the whole fling.
      const float instant = -io.MouseDelta.y / io.DeltaTime;
      scroll.velocity += (instant - scroll.velocity) * 0.4f;
    }
    return;
  }

  // Released: keep scrolling at the finger's speed, easing off.
  if (!scroll.dragging) {
    scroll.window = nullptr;
    return;
  }
  const float y = scroll.window->Scroll.y;
  const bool atEdge = (scroll.velocity < 0.f && y <= 0.f) ||
                      (scroll.velocity > 0.f && y >= scroll.window->ScrollMax.y);
  if (atEdge || std::fabs(scroll.velocity) < g.FontSize) {
    scroll = TouchScroll{};
    return;
  }
  ImGui::SetScrollY(scroll.window, y + scroll.velocity * io.DeltaTime);
  scroll.velocity *= std::exp(-4.f * io.DeltaTime);
}

// A finger that lifts leaves ImGui's cursor where it was, so whatever was last
// tapped stays drawn as hovered. Move the cursor off-screen once the release
// has been seen; queued now, it lands on the next frame.
void ClearTouchHover() {
  ImGuiIO& io = ImGui::GetIO();
  if (io.MouseSource == ImGuiMouseSource_TouchScreen && io.MouseReleased[0] &&
      !ImGui::IsAnyMouseDown()) {
    io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
  }
}

// Full screen inside the display's safe area (clear of the notch and the
// gesture bars), no title bar to drag, a Close button a thumb can hit, and a
// page list down the side in place of a tab strip too narrow to tap.
bool DrawPageWindow() {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImVec2 pos = viewport->WorkPos;
  ImVec2 size = viewport->WorkSize;
  SDL_Window* window = MainWindow();
  SDL_Rect safe;
  int windowWidth = 0, windowHeight = 0;
  if (window != nullptr && SDL_GetWindowSafeArea(window, &safe) &&
      SDL_GetWindowSize(window, &windowWidth, &windowHeight) && windowWidth > 0 &&
      windowHeight > 0 && safe.w > 0 && safe.h > 0) {
    // ImGui's display size need not be in window coordinates.
    const float sx = size.x / static_cast< float >(windowWidth);
    const float sy = size.y / static_cast< float >(windowHeight);
    pos = ImVec2(pos.x + safe.x * sx, pos.y + safe.y * sy);
    size = ImVec2(safe.w * sx, safe.h * sy);
  }
  ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(size, ImGuiCond_Always);
  constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_NoSavedSettings |
                                      ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoScrollWithMouse;
  bool open = true;
  if (ImGui::Begin("Metroid Prime 2 Port##touch", nullptr, kFlags)) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const char* const kClose = "Close";
    const float closeWidth = ImGui::CalcTextSize(kClose).x + style.FramePadding.x * 4.f;
    const char* const kTitle = "Metroid Prime 2 Port";
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(kTitle);
    char fps[32];
    std::snprintf(fps, sizeof(fps), "%.1f fps", static_cast< double >(sFps));
    const float fpsWidth = ImGui::CalcTextSize(fps).x;
    if (ImGui::GetContentRegionAvail().x >
        ImGui::CalcTextSize(kTitle).x + fpsWidth + closeWidth + style.ItemSpacing.x * 5.f) {
      ImGui::SameLine(0.f, style.ItemSpacing.x * 2.f);
      ImGui::TextDisabled("%s", fps);
    }
#if !defined(__ANDROID__)
    // Only when it fits beside the title, the frame rate and the Close button.
    const std::string keyHint = HotkeyHint();
    const char* const kKeys = keyHint.c_str();
    if (ImGui::GetContentRegionAvail().x > ImGui::CalcTextSize(kTitle).x + fpsWidth +
                                               ImGui::CalcTextSize(kKeys).x + closeWidth +
                                               style.ItemSpacing.x * 8.f) {
      ImGui::SameLine(0.f, style.ItemSpacing.x * 3.f);
      ImGui::TextDisabled("%s", kKeys);
    }
#endif
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - closeWidth);
    if (ImGui::Button(kClose, ImVec2(closeWidth, 0.f))) {
      open = false;
    }
    ImGui::Separator();
    DrawSearchField();

    static int sPage = 0;
    if (const int jumpPage = TakeJumpPage(); jumpPage >= 0) {
      sPage = jumpPage;
    }
    // MP2_DEBUG_TAB=<name> opens on that page, for captures of the overlay.
    static bool sStartPending = true;
    if (sStartPending) {
      for (int i = 0; i < kDebugPageCount; ++i) {
        if (SDL_strcasecmp(GetStartTarget().page.c_str(), kDebugPages[i].name) == 0) {
          sPage = i;
        }
      }
      sStartPending = false;
    }
    float listWidth = 0.f;
    for (int i = 0; i < kDebugPageCount; ++i) {
      listWidth = std::max(listWidth, ImGui::CalcTextSize(kDebugPages[i].name).x);
    }
    listWidth += style.FramePadding.x * 2.f + style.WindowPadding.x * 2.f;
    const float rowHeight = ImGui::GetFrameHeight() * 1.2f;
    if (ImGui::BeginChild("##pages", ImVec2(listWidth, 0.f), ImGuiChildFlags_Borders)) {
      for (int i = 0; i < kDebugPageCount; ++i) {
        if (ImGui::Selectable(kDebugPages[i].name, sPage == i && !Searching(),
                              ImGuiSelectableFlags_None, ImVec2(0.f, rowHeight))) {
          sPage = i;
          sSearch[0] = '\0';
        }
      }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    // Keyed by page, so each page keeps its own scroll position.
    const bool searching = Searching();
    ImGui::PushID(searching ? -1 : sPage);
    if (ImGui::BeginChild("##page", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders)) {
      if (searching) {
        DrawSearchResults();
      } else {
        DrawPageBody(kDebugPages[sPage]);
      }
    }
    ImGui::EndChild();
    ImGui::PopID();
  }
  ImGui::End();
  return open;
}

bool DrawDesktopWindow() {
  ImGui::SetNextWindowPos(ImVec2(8.f, 8.f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(440.f, 200.f), ImGuiCond_FirstUseEver);
  if (port::EnvString("MP2_DEBUG_TAB") != nullptr) // a capture wants to see the tab
    ImGui::SetNextWindowSize(ImVec2(520.f, 620.f), ImGuiCond_Once);
  bool open = true;
  if (ImGui::Begin("Metroid Prime 2 Port", &open, ImGuiWindowFlags_MenuBar)) {
    if (ImGui::BeginMenuBar()) {
      ImGui::TextUnformatted(HotkeyHint().c_str());
      const ImGuiStyle& style = ImGui::GetStyle();
      ImGui::SameLine(0.f, style.ItemSpacing.x * 3.f);
      ImGui::TextDisabled("%.1f fps", static_cast< double >(sFps));
      ImGui::EndMenuBar();
    }

    DrawSearchField();
    if (Searching()) {
      if (ImGui::BeginChild("##search-results")) {
        DrawSearchResults();
      }
      ImGui::EndChild();
    } else if (ImGui::BeginTabBar("##debug_tabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
      // MP2_DEBUG_TAB=<name> opens on that tab, for captures of the overlay.
      static bool sStartPending = true;
      const int jumpPage = TakeJumpPage();
      for (int i = 0; i < kDebugPageCount; ++i) {
        const DebugPage& page = kDebugPages[i];
        const bool start =
            sStartPending && SDL_strcasecmp(GetStartTarget().page.c_str(), page.name) == 0;
        if (ImGui::BeginTabItem(page.name, nullptr,
                                start || i == jumpPage ? ImGuiTabItemFlags_SetSelected : 0)) {
          if (start)
            sStartPending = false;
          DrawPageBody(page);
          ImGui::EndTabItem();
        }
      }
      ImGui::EndTabBar();
    }
  }
  ImGui::End();
  return open;
}

} // namespace

std::string HotkeyHint() {
  std::string hint;
  const std::string menu = PortInputRemap::BindingsText(PortInput::Action::PortMenu, " / ");
  if (!menu.empty()) hint += menu + ": hide   ";
  hint += "F10: frame limit";
  const std::string shot = PortInputRemap::BindingsText(PortInput::Action::Screenshot, " / ");
  if (!shot.empty()) hint += "   " + shot + ": screenshot";
  return hint;
}

// After the frame's settings are applied (PortOverlay::Draw): saves a changed setting, feeds the
// pad to ImGui, and draws the window while it is open.
void DrawUI() {
  // Not while a control is held: a dragged slider changes a setting every frame. It is saved the
  // frame the control is let go.
  if (PortOverlaySettings::Dirty() && ImGui::GetCurrentContext() != nullptr && !ImGui::IsAnyItemActive()) {
    PortOverlaySettings::SaveSettings();
  }
  UpdateControllerNav();
  // While the window is open the cursor belongs to it, not to the game's aim.
  PortMouse::SetUiActive(sVisible);
  BeginTopToastStack();
  DrawMenuHintToast();
  DrawShaderCompilationToast();
  if (!sVisible) {
    sTouchScroll = TouchScroll{};
    return;
  }
  UpdateUiScale(static_cast< float >(PortOverlay::UiScale()) / 100.f);
  ApplyThemeIfChanged();

  UpdateTouchScroll();
  BeginSearchJump();
  const bool open = PageLayout() ? DrawPageWindow() : DrawDesktopWindow();
  EndSearchJump();
  ClearTouchHover();

  if (!open) {
    sVisible = false;
  }
}

} // namespace PortDebug
