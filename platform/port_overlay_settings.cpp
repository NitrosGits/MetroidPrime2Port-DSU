// F1 overlay: the settings file: one table of keys (kSettingKeys) that reads and writes
// <user>/settings.cfg, saving it, and the settings self-test. A port of MP1's
// debug_ui/settings.cpp; the keys and the variables they reach are MP2's. State shared with
// port_overlay.cpp is in port_overlay_internal.h.

#include "port_overlay_internal.h"

#include "port_env.h"
#include "port_overlay.h"
#include "port_settings.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <string>

using namespace PortOverlayState;

namespace {

std::string Trim(const std::string& s) {
  size_t a = 0;
  size_t b = s.size();
  while (a < b && std::isspace(static_cast< unsigned char >(s[a])))
    ++a;
  while (b > a && std::isspace(static_cast< unsigned char >(s[b - 1])))
    --b;
  return s.substr(a, b - a);
}

bool ParseBool(const std::string& value) {
  std::string lower;
  for (char c : value)
    lower += static_cast< char >(std::tolower(static_cast< unsigned char >(c)));
  return !(lower.empty() || lower == "0" || lower == "false" || lower == "off" || lower == "no");
}

// A whole-string integer; false for anything else.
bool ParseInt(const std::string& value, int& out) {
  char* end = nullptr;
  const long parsed = std::strtol(value.c_str(), &end, 10);
  if (value.empty() || end == value.c_str() || *end != '\0')
    return false;
  out = static_cast< int >(parsed);
  return true;
}

// A whole-string float; false for anything else, NaN and infinity included.
bool ParseFloat(const std::string& value, float& out) {
  char* end = nullptr;
  const float parsed = std::strtof(value.c_str(), &end);
  if (value.empty() || end == value.c_str() || *end != '\0' || !std::isfinite(parsed))
    return false;
  out = parsed;
  return true;
}

// Three comma-separated floats, each clamped to [min, max].
bool ParseFloat3(const std::string& value, float out[3], float min, float max) {
  float parsed[3];
  char extra = 0;
  if (std::sscanf(value.c_str(), "%f,%f,%f%c", &parsed[0], &parsed[1], &parsed[2], &extra) != 3)
    return false;
  for (float v : parsed) {
    if (!std::isfinite(v))
      return false;
  }
  for (int i = 0; i < 3; ++i)
    out[i] = std::clamp(parsed[i], min, max);
  return true;
}

// One key of settings.cfg: how its value is read and how it is written.
// The table is the file's order. ApplySetting reads through it, SettingsText
// writes through it, and SettingsSelfTest checks that each key's write reads
// back as the same value.
struct SettingKey {
  const char* key;
  // A value that does not parse or is out of range is ignored.
  void (*read)(const std::string& value);
  // The value only, after "key="; nullptr for an old name that is only read.
  void (*write)(std::ostream& out);
  // Write no line while the value is empty.
  bool omitEmpty;
  // Two different values as written, which the self-test applies in turn and
  // expects back; nullptr when the written form is not the one read.
  const char* sampleA;
  const char* sampleB;
};

void WriteBool(std::ostream& out, bool value) { out << (value ? 1 : 0); }

// A key that is one bool: read with ParseBool, written as 1 or 0, sampled with both.
template < bool* Flag >
constexpr SettingKey BoolKey(const char* key) {
  return {key,
          [](const std::string& value) { *Flag = ParseBool(value); },
          [](std::ostream& out) { WriteBool(out, *Flag); },
          false,
          "1",
          "0"};
}

// A key that is one int in [Min, Max]: anything else is ignored.
template < int* Value, int Min, int Max >
constexpr SettingKey IntKey(const char* key, const char* sampleA, const char* sampleB) {
  return {key,
          [](const std::string& value) {
            int parsed = 0;
            if (ParseInt(value, parsed) && parsed >= Min && parsed <= Max)
              *Value = parsed;
          },
          [](std::ostream& out) { out << *Value; },
          false,
          sampleA,
          sampleB};
}

const SettingKey kSettingKeys[] = {
    BoolKey< &sSkipCinematics >("skip_cinematics"),
    BoolKey< &sFastMorph >("fast_morph"),
    BoolKey< &sSpringBall >("spring_ball"),
    BoolKey< &sLockOnToggle >("lockon_toggle"),
    BoolKey< &sStickyCharge >("sticky_charge"),
    BoolKey< &sRapidCharge >("rapid_charge"),
    BoolKey< &sMouseButtons >("mouse_buttons"),
    BoolKey< &sCardBackup >("card_backup"),
    BoolKey< &sLogFile >("log_file"),
    BoolKey< &sHideHelmet >("no_helmet"),
    BoolKey< &sHideVisorEffects >("no_visor_fx"),
    // 0 keeps the game's own FOV.
    {"fov",
     [](const std::string& value) {
       float parsed = 0.f;
       if (ParseFloat(value, parsed) && (parsed == 0.f || (parsed >= 30.f && parsed <= 120.f))) {
         sFirstPersonFov = parsed;
       }
     },
     [](std::ostream& out) { out << sFirstPersonFov; }, false, "45", "72.5"},
    {"gun_offset",
     [](const std::string& value) {
       ParseFloat3(value, sGunOffset, -PortOverlay::kGunOffsetMax, PortOverlay::kGunOffsetMax);
     },
     [](std::ostream& out) {
       out << sGunOffset[0] << ',' << sGunOffset[1] << ',' << sGunOffset[2];
     },
     false, "0.5,-0.25,1", "0,0,0"},
    // MSAA is 1 or 4.
    {"msaa",
     [](const std::string& value) {
       int parsed = 0;
       if (ParseInt(value, parsed) && (parsed == 1 || parsed == 4))
         sMsaa = parsed;
     },
     [](std::ostream& out) { out << sMsaa; }, false, "4", "1"},
    // A max, not a step: any value in range is kept, and Aurora clamps what a sampler asks for.
    IntKey< &sAnisotropy, 1, 16 >("aniso", "8", "16"),
    // 0 means the full-size HUD.
    {"hud_scale",
     [](const std::string& value) {
       int parsed = 0;
       if (!ParseInt(value, parsed))
         return;
       if (parsed == 0)
         parsed = PortOverlay::kHudScaleMax;
       if (parsed >= PortOverlay::kHudScaleMin && parsed <= PortOverlay::kHudScaleMax)
         sHudScale = parsed;
     },
     [](std::ostream& out) { out << sHudScale; }, false, "50", "100"},
    IntKey< &sUiScale, PortOverlay::kUiScaleMin, PortOverlay::kUiScaleMax >("ui_scale", "50",
                                                                            "150"),
    BoolKey< &sVsync >("vsync"),
    BoolKey< &sOverlayWindowed >("overlay_windowed"),
    {"ui_theme",
     [](const std::string& value) {
       sUiTheme = value == "prime" ? 1 : value == "plain" ? 2 : 0;
     },
     [](std::ostream& out) { out << (sUiTheme == 1 ? "prime" : sUiTheme == 2 ? "plain" : "auto"); },
     false, "plain", "auto"},
    BoolKey< &sAimCrosshair >("aim_crosshair"),
    IntKey< &sCrosshairStyle, PortOverlay::kCrosshairStyle_Classic,
            PortOverlay::kCrosshairStyle_Remastered >("crosshair_style", "0", "1"),
    IntKey< &sCrosshairSize, PortOverlay::kCrosshairSizeMin, PortOverlay::kCrosshairSizeMax >(
        "crosshair_size", "25", "100"),
    IntKey< &sCrosshairOpacity, PortOverlay::kCrosshairOpacityMin,
            PortOverlay::kCrosshairOpacityMax >("crosshair_opacity", "0", "100"),
    {"crosshair_color",
     [](const std::string& value) { ParseFloat3(value, sCrosshairColor, 0.f, 1.f); },
     [](std::ostream& out) {
       out << sCrosshairColor[0] << ',' << sCrosshairColor[1] << ',' << sCrosshairColor[2];
     },
     false, "1,0.5,0", "0,0.25,1"},
    BoolKey< &sCrosshairCustomColor >("crosshair_custom_color"),
};

std::mutex sMutex;
std::atomic< bool > sSettingsDirty{false};
bool sLoaded = false;
// The text last read from or written to the file: SaveSettings writes only when SettingsText()
// differs.
std::string sBaseline;

} // namespace

namespace PortOverlaySettings {

bool ApplySetting(const std::string& key, const std::string& value) {
  for (const SettingKey& entry : kSettingKeys) {
    if (entry.key != nullptr && key == entry.key) {
      entry.read(value);
      return true;
    }
  }
  return false;
}

// Applies the key=value lines of a settings file. Only whole-line comments:
// values are written raw.
void ApplySettingsText(std::istream& text) {
  std::string line;
  while (std::getline(text, line)) {
    const size_t first = line.find_first_not_of(" \t");
    if (first == std::string::npos || line[first] == '#') {
      continue;
    }
    const size_t separator = line.find('=');
    if (separator == std::string::npos) {
      continue;
    }
    const std::string key = Trim(line.substr(0, separator));
    const std::string value = Trim(line.substr(separator + 1));
    if (!key.empty()) {
      ApplySetting(key, value);
    }
  }
}

// key=value lines of a settings text, comments skipped.
std::map< std::string, std::string > SettingsValues(const std::string& text) {
  std::map< std::string, std::string > values;
  std::istringstream lines(text);
  std::string line;
  while (std::getline(lines, line)) {
    const size_t separator = line.find('=');
    if (!line.empty() && line[0] != '#' && separator != std::string::npos) {
      values[line.substr(0, separator)] = line.substr(separator + 1);
    }
  }
  return values;
}

// The settings file's text, as written by SaveSettings.
std::string SettingsText() {
  std::ostringstream file;
  file << "# Metroid Prime 2 native port settings: <user>/settings.cfg. Written by the F1 "
          "overlay.\n";
  file << "# Environment variables (MP2_<KEY>) override these for a single run.\n";
  for (const SettingKey& entry : kSettingKeys) {
    if (entry.write == nullptr) {
      continue;
    }
    std::ostringstream value;
    entry.write(value);
    if (entry.omitEmpty && value.str().empty()) {
      continue;
    }
    file << entry.key << '=' << value.str() << '\n';
  }
  return file.str();
}

void LoadSettings() {
  std::lock_guard< std::mutex > lock(sMutex);
  if (sLoaded) {
    return;
  }
  sLoaded = true;
  std::string text;
  if (PortSettings::ReadFile(text)) {
    std::fprintf(stderr, "port: loaded settings from %s\n", PortSettings::FilePath().c_str());
    std::istringstream lines(text);
    ApplySettingsText(lines);
  }
  sBaseline = SettingsText();
}

// One log line per key whose value differs between the old and new settings text (MP1's
// LogSettingChanges); the old text is the baseline, so only real changes are named.
void LogSettingChanges(const std::string& before, const std::string& after) {
  const std::map< std::string, std::string > old = SettingsValues(before);
  for (const auto& [key, value] : SettingsValues(after)) {
    const auto it = old.find(key);
    if (it == old.end() || it->second != value) {
      std::fprintf(stderr, "port: setting %s: %s -> %s\n", key.c_str(),
                   it == old.end() ? "(unset)" : it->second.c_str(), value.c_str());
    }
  }
}

void SaveSettings() {
  std::lock_guard< std::mutex > lock(sMutex);
  if (!sLoaded || !sSettingsDirty.load() || PortSettings::FilePath().empty()) {
    return;
  }
  sSettingsDirty = false;
  const std::string text = SettingsText();
  if (text == sBaseline) {
    return;
  }
  if (!PortSettings::WriteFile(text)) {
    sSettingsDirty = true;
    return;
  }
  LogSettingChanges(sBaseline, text);
  std::fprintf(stderr, "port: saved settings to %s\n", PortSettings::FilePath().c_str());
  sBaseline = text;
}

void MarkDirty() { sSettingsDirty = true; }

bool Dirty() { return sSettingsDirty.load(); }

std::string EnvName(const std::string& key) {
  if (key == "lockon_toggle") {
    return "MP2_LOCK_ON_TOGGLE";
  }
  std::string name = "MP2_";
  for (char c : key)
    name += static_cast< char >(std::toupper(static_cast< unsigned char >(c)));
  return name;
}

int SettingsSelfTest(std::string& report) {
  int failures = 0;
  int sampled = 0;
  char line[512];
  // Each sampled key, through the file reader: both values must come back as written.
  for (const SettingKey& entry : kSettingKeys) {
    if (entry.key == nullptr || entry.write == nullptr || entry.sampleA == nullptr) {
      continue;
    }
    ++sampled;
    for (const char* sample : {entry.sampleA, entry.sampleB}) {
      std::istringstream in(std::string(entry.key) + "=" + sample + "\n");
      ApplySettingsText(in);
      const auto written = SettingsValues(SettingsText());
      const auto found = written.find(entry.key);
      const std::string got = found == written.end() ? std::string() : found->second;
      if (got != sample) {
        std::snprintf(line, sizeof(line), "settings self-test: %s=%s reads back as %s=%s\n",
                      entry.key, sample, entry.key, got.c_str());
        report += line;
        ++failures;
      }
    }
  }
  // Every line written is a key something reads, and reading the whole file
  // back writes it unchanged.
  const std::string first = SettingsText();
  for (const auto& [key, value] : SettingsValues(first)) {
    if (!ApplySetting(key, value)) {
      report += "settings self-test: " + key + " is written but nothing reads it\n";
      ++failures;
    }
  }
  std::istringstream again(first);
  ApplySettingsText(again);
  const std::string second = SettingsText();
  if (second != first) {
    report +=
        "settings self-test: the file changes when read back\n" + first + "---\n" + second + "\n";
    ++failures;
  }
  std::snprintf(line, sizeof(line),
                "settings self-test: %d keys sampled, %zu lines round-tripped, %d failure(s)\n",
                sampled, SettingsValues(first).size(), failures);
  report += line;
  return failures == 0 ? 0 : 1;
}

} // namespace PortOverlaySettings
