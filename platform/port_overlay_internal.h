// The F1 overlay's live settings and the settings file's functions, shared by port_overlay.cpp
// (which owns the variables), debug_ui/settings.cpp (the key table, MP1's debug_ui/settings.cpp
// ported) and the pages in debug_ui/. The variables hold the file's and the UI's values; an
// `MP2_<KEY>` environment variable is applied when a value is read (the accessors in
// port_overlay.cpp), never stored here.
#pragma once

#include <iosfwd>
#include <map>
#include <string>

namespace PortOverlayState {

extern bool sSkipCinematics;
extern int sBootLogos;
extern int sWorldNameCard;
extern bool sLogFile;
extern bool sFastMorph;
extern bool sSpringBall;
extern bool sLockOnToggle;
extern bool sStickyCharge;
extern bool sRapidCharge;
extern bool sAutoAim;
extern bool sPointerAim;
extern int sPointerAimBox;
extern int sPointerAimTurn;
extern bool sGyroAim;
extern int sGyroSource;
extern int sGyroSpace;
extern int sGyroAutoCalibrate;
extern float sGyroSmoothing;
extern float sGyroTightening;
extern float sGyroCursorAngle;
extern int sGyroMode;
extern float sGyroSensitivity;
extern bool sGyroCursorCamera;
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
extern bool sFullscreen;
extern bool sFrameLimit;
extern bool sSmoothFrames;
extern bool sPoseInterpolation;
extern bool sParticleInterpolation;
extern bool sLookInterpolation;
extern bool sHudWide;
extern float sRenderScale;
extern int sAspectMode;
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
// Reads <user>/settings.cfg once per run (a later call does nothing; one before
// PortSettings::Initialize is ignored, and main.cpp calls it right after). Never writes.
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
