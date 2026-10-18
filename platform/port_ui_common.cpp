// See port_ui_common.h. ApplyTouchStyle and ApplyPrimeTheme/FillThemeColors are the Prime 1 port's
// (MetroidPrimePort/platform/debug_ui/ui_common.cpp), reduced to the Prime theme.
#include "port_ui_common.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace PortUi {
namespace {

struct Rgb {
  float x, y, z;
};

struct Palette {
  Rgb bg;
  float bgAlpha;
  Rgb deep;     // child / title / menu background
  Rgb frame;    // frames, buttons, tabs
  Rgb frameHi;  // hovered frames
  Rgb accent;
  Rgb text;
  Rgb textDim;
};

ImVec4 Rgba(const Rgb& c, float a) { return ImVec4(c.x, c.y, c.z, a); }

// Every ImGuiCol_ is derived from the palette so none is left at the stock value.
void FillThemeColors(ImGuiStyle& style, const Palette& p) {
  ImVec4* c = style.Colors;
  const Rgb a = p.accent;
  c[ImGuiCol_Text] = Rgba(p.text, 1.f);
  c[ImGuiCol_TextDisabled] = Rgba(p.textDim, 1.f);
  c[ImGuiCol_WindowBg] = Rgba(p.bg, p.bgAlpha);
  c[ImGuiCol_ChildBg] = Rgba(p.deep, 0.55f);
  c[ImGuiCol_PopupBg] = Rgba(p.bg, 0.97f);
  c[ImGuiCol_Border] = Rgba(a, 0.55f);
  c[ImGuiCol_BorderShadow] = ImVec4(0.f, 0.f, 0.f, 0.f);
  c[ImGuiCol_FrameBg] = Rgba(p.frame, 0.90f);
  c[ImGuiCol_FrameBgHovered] = Rgba(p.frameHi, 0.95f);
  c[ImGuiCol_FrameBgActive] = Rgba(a, 0.35f);
  c[ImGuiCol_TitleBg] = Rgba(p.deep, 1.f);
  c[ImGuiCol_TitleBgActive] = Rgba(p.frame, 1.f);
  c[ImGuiCol_TitleBgCollapsed] = Rgba(p.deep, 0.75f);
  c[ImGuiCol_MenuBarBg] = Rgba(p.deep, 1.f);
  c[ImGuiCol_ScrollbarBg] = ImVec4(0.f, 0.f, 0.f, 0.30f);
  c[ImGuiCol_ScrollbarGrab] = Rgba(p.frameHi, 1.f);
  c[ImGuiCol_ScrollbarGrabHovered] = Rgba(a, 0.60f);
  c[ImGuiCol_ScrollbarGrabActive] = Rgba(a, 0.90f);
  c[ImGuiCol_CheckMark] = Rgba(a, 1.f);
  c[ImGuiCol_SliderGrab] = Rgba(a, 0.85f);
  c[ImGuiCol_SliderGrabActive] = Rgba(a, 1.f);
  c[ImGuiCol_Button] = Rgba(p.frame, 0.90f);
  c[ImGuiCol_ButtonHovered] = Rgba(p.frameHi, 1.f);
  c[ImGuiCol_ButtonActive] = Rgba(a, 0.55f);
  c[ImGuiCol_Header] = Rgba(p.frame, 0.80f);
  c[ImGuiCol_HeaderHovered] = Rgba(a, 0.35f);
  c[ImGuiCol_HeaderActive] = Rgba(a, 0.55f);
  c[ImGuiCol_Separator] = Rgba(a, 0.35f);
  c[ImGuiCol_SeparatorHovered] = Rgba(a, 0.70f);
  c[ImGuiCol_SeparatorActive] = Rgba(a, 1.f);
  c[ImGuiCol_ResizeGrip] = Rgba(a, 0.30f);
  c[ImGuiCol_ResizeGripHovered] = Rgba(a, 0.65f);
  c[ImGuiCol_ResizeGripActive] = Rgba(a, 0.90f);
  c[ImGuiCol_TabHovered] = Rgba(a, 0.50f);
  c[ImGuiCol_Tab] = Rgba(p.frame, 0.85f);
  c[ImGuiCol_TabSelected] = Rgba(a, 0.40f);
  c[ImGuiCol_TabSelectedOverline] = Rgba(a, 1.f);
  c[ImGuiCol_TabDimmed] = Rgba(p.deep, 0.90f);
  c[ImGuiCol_TabDimmedSelected] = Rgba(a, 0.25f);
#ifdef IMGUI_HAS_DOCK
  c[ImGuiCol_TabDimmedSelectedOverline] = Rgba(a, 0.55f);
  c[ImGuiCol_DockingPreview] = Rgba(a, 0.45f);
  c[ImGuiCol_DockingEmptyBg] = Rgba(p.deep, 1.f);
#endif
  c[ImGuiCol_PlotLines] = Rgba(a, 0.90f);
  c[ImGuiCol_PlotLinesHovered] = Rgba(p.text, 1.f);
  c[ImGuiCol_PlotHistogram] = Rgba(a, 0.80f);
  c[ImGuiCol_PlotHistogramHovered] = Rgba(p.text, 1.f);
  c[ImGuiCol_TableHeaderBg] = Rgba(p.frame, 1.f);
  c[ImGuiCol_TableBorderStrong] = Rgba(a, 0.50f);
  c[ImGuiCol_TableBorderLight] = Rgba(a, 0.20f);
  c[ImGuiCol_TableRowBg] = ImVec4(0.f, 0.f, 0.f, 0.f);
  c[ImGuiCol_TableRowBgAlt] = ImVec4(1.f, 1.f, 1.f, 0.035f);
  c[ImGuiCol_TextLink] = Rgba(a, 1.f);
  c[ImGuiCol_TextSelectedBg] = Rgba(a, 0.35f);
  c[ImGuiCol_DragDropTarget] = Rgba(a, 0.90f);
  c[ImGuiCol_NavCursor] = Rgba(a, 1.f);
  c[ImGuiCol_NavWindowingHighlight] = ImVec4(1.f, 1.f, 1.f, 0.70f);
  c[ImGuiCol_NavWindowingDimBg] = Rgba(p.bg, 0.55f);
  c[ImGuiCol_ModalWindowDimBg] = Rgba(p.bg, 0.60f);
}

// Paddings and sizes, unscaled.
void ApplyTouchStyle(ImGuiStyle& style) {
  style.WindowPadding = ImVec2(10.f, 10.f);
  style.FramePadding = ImVec2(10.f, 7.f);
  style.ItemSpacing = ImVec2(10.f, 8.f);
  style.ItemInnerSpacing = ImVec2(8.f, 6.f);
  style.CellPadding = ImVec2(6.f, 4.f);
  style.TouchExtraPadding = ImVec2(3.f, 3.f);
  style.IndentSpacing = 22.f;
  style.ScrollbarSize = 18.f;
  style.GrabMinSize = 18.f;
  style.FrameRounding = 5.f;
  style.GrabRounding = 5.f;
  style.ScrollbarRounding = 9.f;
  style.TabRounding = 5.f;
}

// Retail pause / scan visor: near-black blue, teal frames, amber accent. Borders and rounding in
// unscaled units times `scale` (ScaleAllSizes has already run for the paddings).
void ApplyPrimeTheme(ImGuiStyle& style, float scale) {
  Palette p;
  p.bg = {0.020f, 0.063f, 0.102f};  // #05101A
  p.bgAlpha = 0.92f;
  p.deep = {0.030f, 0.090f, 0.140f};
  p.frame = {0.045f, 0.170f, 0.230f};
  p.frameHi = {0.070f, 0.260f, 0.340f};
  p.accent = {1.000f, 0.604f, 0.180f};  // #FF9A2E
  p.text = {0.95f, 0.91f, 0.82f};
  p.textDim = {0.45f, 0.55f, 0.65f};
  FillThemeColors(style, p);
  style.WindowBorderSize = style.ChildBorderSize = style.PopupBorderSize = style.FrameBorderSize =
      std::max(1.f, std::round(scale));
  style.TabBorderSize = 0.f;
  style.WindowRounding = style.ChildRounding = style.PopupRounding = 2.f * scale;
  style.FrameRounding = style.GrabRounding = style.TabRounding = 2.f * scale;
  style.ScrollbarRounding = 2.f * scale;
}

#if defined(__ANDROID__)
constexpr bool kThemed = true;
#else
constexpr bool kThemed = false;
#endif

// The env var and description under a control (themed), or beside/under it (the original look).
void Lines(const char* env, const char* description, bool sameLine) {
  if (kThemed) {
    if (env != nullptr) {
      Help(env);
    }
    if (description != nullptr) {
      Help(description);
    }
    return;
  }
  if (env != nullptr) {
    if (sameLine) {
      ImGui::SameLine();
    }
    ImGui::TextDisabled("(%s)", env);
  }
  if (description != nullptr) {
    ImGui::TextDisabled("%s", description);
  }
}

} // namespace

void ApplyTheme(float displayScale) {
  if (!kThemed || ImGui::GetCurrentContext() == nullptr) {
    return;
  }
  static float sApplied = 0.f;
  const float scale = std::clamp(displayScale, 1.f, 4.f);
  if (sApplied == scale) {
    return;
  }
  ImGuiStyle& style = ImGui::GetStyle();
  ImGuiIO& io = ImGui::GetIO();
  if (sApplied != 0.f) {
    io.FontGlobalScale /= sApplied;
  }
  // Rebuilt from the unscaled values, so a changed scale never compounds.
  ApplyTouchStyle(style);
  style.ScaleAllSizes(scale);
  io.FontGlobalScale *= scale;
  ApplyPrimeTheme(style, scale);
  sApplied = scale;
}

void Help(const char* text) {
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
  ImGui::TextWrapped("%s", text);
  ImGui::PopStyleColor();
}

void Note(const char* env, const char* description, bool sameLine) {
  Lines(env, description, sameLine);
}

bool Toggle(const char* label, bool* value, const char* env, const char* description) {
  const bool changed = ImGui::Checkbox(label, value);
  Lines(env, description, true);
  return changed;
}

bool SliderFloat(const char* label, float* value, float min, float max, const char* format,
                 const char* env, const char* description) {
  const bool changed = ImGui::SliderFloat(label, value, min, max, format);
  Lines(env, description, false);
  return changed;
}

bool SliderInt(const char* label, int* value, int min, int max, const char* format,
               const char* env, const char* description) {
  const bool changed = ImGui::SliderInt(label, value, min, max, format);
  Lines(env, description, false);
  return changed;
}

bool Combo(const char* label, int* current, const char* items, const char* env,
           const char* description) {
  const bool changed = ImGui::Combo(label, current, items);
  Lines(env, description, true);
  return changed;
}

} // namespace PortUi
