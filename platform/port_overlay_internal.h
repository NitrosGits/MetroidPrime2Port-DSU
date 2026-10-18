// The F1 overlay's live settings and the settings file's functions, shared by port_overlay.cpp
// (which owns the variables and the UI) and port_overlay_settings.cpp (the key table, MP1's
// debug_ui/settings.cpp ported). The variables hold the file's and the UI's values; an `MP2_<KEY>`
// environment variable is applied when a value is read (the accessors in port_overlay.cpp), never
// stored here.
#pragma once

#include <iosfwd>
#include <map>
#include <string>

namespace PortOverlayState {

extern bool sSkipCinematics;
extern bool sLogFile;
extern bool sFastMorph;
extern bool sSpringBall;
extern bool sLockOnToggle;
extern bool sStickyCharge;
extern bool sMouseButtons;
extern bool sCardBackup;
extern float sFirstPersonFov;
extern bool sHideHelmet;
extern bool sHideVisorEffects;
extern bool sAimCrosshair;
extern int sCrosshairStyle;
extern int sCrosshairSize;
extern int sCrosshairOpacity;
extern bool sCrosshairCustomColor;
extern float sCrosshairColor[3];
extern float sGunOffset[3];
extern int sMsaa;
extern int sAnisotropy;
extern int sHudScale;
extern int sUiScale;
extern bool sVsync;
extern bool sOverlayWindowed;
extern int sUiTheme;

} // namespace PortOverlayState

namespace PortOverlaySettings {

// False for a key nothing reads.
bool ApplySetting(const std::string& key, const std::string& value);
// Applies the key=value lines of a settings file.
void ApplySettingsText(std::istream& text);
// key=value lines of a settings text, comments skipped.
std::map< std::string, std::string > SettingsValues(const std::string& text);
// The settings file's text, as SaveSettings writes it.
std::string SettingsText();
// Reads <user>/settings.cfg once per run (a later call does nothing). Never writes.
void LoadSettings();
// Writes the file when a setter or row marked it dirty and the text now differs from what was last
// read or saved.
void SaveSettings();
void MarkDirty();
bool Dirty();
// The environment variable that overrides `key`.
std::string EnvName(const std::string& key);
// Writes every key and reads it back; the report lines go to `report`. Returns 0 when all pass. It
// leaves the live variables as the table's last samples: the caller restores them.
int SettingsSelfTest(std::string& report);

} // namespace PortOverlaySettings

// The F1 window's pages and the settings search, the Prime 1 port's debug_ui/search.cpp ported
// (port_overlay_search.cpp). The pages are port_overlay.cpp's.
namespace PortOverlaySearch {

struct Page {
  const char* name;
  void (*draw)();
};
extern const Page kPages[];
extern const int kPageCount;

// One row of port_settings_index.inc.
struct SettingsIndexRow {
  const char* page;
  const char* sub;
  const char* section;
  const char* label;
  const char* keywords;
};
extern char sSearch[128];
// The page a search result asked for, once (-1 when none).
int TakeJumpPage();
// A page's inner tab (BeginTabItem), opened when a search result names it; end with EndTabItem.
bool SubTab(const char* page, const char* name);
// Draws a page's rows, where a jump looks for its widget.
void DrawPageBody(const Page& page);
// Around the window: install and remove the item hooks a jump finds its widget with, and flash it.
void BeginSearchJump();
void EndSearchJump();
void DrawSearchField();
bool Searching();
void DrawSearchResults();

} // namespace PortOverlaySearch
