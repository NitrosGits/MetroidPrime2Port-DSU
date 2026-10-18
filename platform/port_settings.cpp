#include "port_settings.h"

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>

namespace PortSettings {
namespace {

enum class Kind { Bool, Int, Float, Float3 };

struct Entry {
  std::string key;
  Kind kind = Kind::Bool;
  bool* b = nullptr;
  int* i = nullptr;
  float* f = nullptr;
  int minI = 0, maxI = 0, zeroAs = kNoZeroAlias;
  bool (*valid)(int) = nullptr;
  float minF = 0.f, maxF = 0.f;
  bool allowZero = false;
  const char* source = "default";
  // The serialised value this run started with (or last saved): a line is rewritten only when the
  // live value no longer matches it.
  std::string baseline;
};

std::mutex sMutex;
std::string sPath;                 // empty: no file
std::string sOriginal;             // the file's text as read (empty when absent)
std::vector<std::unique_ptr<Entry>> sEntries;
bool sDirty = false;

std::string Trim(const std::string& s) {
  size_t a = 0, b = s.size();
  while (a < b && std::isspace(static_cast< unsigned char >(s[a]))) ++a;
  while (b > a && std::isspace(static_cast< unsigned char >(s[b - 1]))) --b;
  return s.substr(a, b - a);
}

// Splits "key = value" (no comment, not blank) into its parts.
bool SplitLine(const std::string& line, std::string& key, std::string& value) {
  const std::string t = Trim(line);
  if (t.empty() || t[0] == '#') return false;
  const size_t eq = t.find('=');
  if (eq == std::string::npos) return false;
  key = Trim(t.substr(0, eq));
  value = Trim(t.substr(eq + 1));
  return !key.empty();
}

std::vector<std::string> SplitLines(const std::string& text) {
  std::vector<std::string> lines;
  std::string cur;
  for (char c : text) {
    if (c == '\n') {
      lines.push_back(cur);
      cur.clear();
    } else if (c != '\r') {
      cur += c;
    }
  }
  if (!cur.empty()) lines.push_back(cur);
  return lines;
}

// The last value the file gives for `key`.
bool FileValue(const std::string& key, std::string& out) {
  bool found = false;
  for (const std::string& line : SplitLines(sOriginal)) {
    std::string k, v;
    if (SplitLine(line, k, v) && k == key) {
      out = v;
      found = true;
    }
  }
  return found;
}

std::string EnvName(const char* key) {
  std::string name = "MP2_";
  for (const char* p = key; *p; ++p) name += static_cast< char >(std::toupper(static_cast< unsigned char >(*p)));
  return name;
}

bool ParseBool(const std::string& text, bool lenient, bool& out) {
  std::string t;
  for (char c : text) t += static_cast< char >(std::tolower(static_cast< unsigned char >(c)));
  if (t == "0" || t == "false" || t == "off" || t == "no") { out = false; return true; }
  if (t == "1" || t == "true" || t == "on" || t == "yes") { out = true; return true; }
  if (lenient && !t.empty()) { out = true; return true; } // MP2_X=anything non-zero always meant on
  return false;
}

bool ParseInt(const std::string& text, bool lenient, int& out) {
  if (text.empty()) return false;
  char* end = nullptr;
  errno = 0;
  const long v = std::strtol(text.c_str(), &end, 10);
  if (end == text.c_str() || errno != 0 || v < INT_MIN || v > INT_MAX) return false;
  if (!lenient && *end != '\0') return false;
  out = static_cast< int >(v);
  return true;
}

bool ParseFloat(const std::string& text, bool lenient, float& out) {
  if (text.empty()) return false;
  char* end = nullptr;
  const double v = std::strtod(text.c_str(), &end);
  if (end == text.c_str() || !std::isfinite(v)) return false;
  if (!lenient && *end != '\0') return false;
  out = static_cast< float >(v);
  return true;
}

std::string Serialise(const Entry& e) {
  char buf[96];
  switch (e.kind) {
  case Kind::Bool:
    return *e.b ? "1" : "0";
  case Kind::Int:
    std::snprintf(buf, sizeof(buf), "%d", *e.i);
    return buf;
  case Kind::Float:
    std::snprintf(buf, sizeof(buf), "%g", static_cast< double >(*e.f));
    return buf;
  case Kind::Float3:
    std::snprintf(buf, sizeof(buf), "%g,%g,%g", static_cast< double >(e.f[0]),
                  static_cast< double >(e.f[1]), static_cast< double >(e.f[2]));
    return buf;
  }
  return {};
}

// Parses `text` for `e` and, only if it is valid, stores it. `lenient` is the environment's
// historical atoi/atof leniency.
bool Apply(Entry& e, const std::string& text, bool lenient) {
  switch (e.kind) {
  case Kind::Bool: {
    bool v;
    if (!ParseBool(text, lenient, v)) return false;
    *e.b = v;
    return true;
  }
  case Kind::Int: {
    int v;
    if (!ParseInt(text, lenient, v)) return false;
    if (v == 0 && e.zeroAs != kNoZeroAlias) v = e.zeroAs;
    if (v < e.minI || v > e.maxI || (e.valid != nullptr && !e.valid(v))) return false;
    *e.i = v;
    return true;
  }
  case Kind::Float: {
    float v;
    if (!ParseFloat(text, lenient, v)) return false;
    if (!(v >= e.minF && v <= e.maxF) && !(e.allowZero && v == 0.f)) return false;
    *e.f = v;
    return true;
  }
  case Kind::Float3: {
    float v[3];
    char extra;
    if (std::sscanf(text.c_str(), "%f,%f,%f%c", &v[0], &v[1], &v[2], &extra) != 3) return false;
    for (float& c : v) {
      if (!std::isfinite(c)) return false;
      c = std::fmin(std::fmax(c, e.minF), e.maxF);
    }
    e.f[0] = v[0]; e.f[1] = v[1]; e.f[2] = v[2];
    return true;
  }
  }
  return false;
}

// Resolves a freshly filled-in entry: default (already in the variable), file, environment.
void Resolve(Entry& e) {
  std::string text;
  if (FileValue(e.key, text) && Apply(e, text, false)) {
    e.source = "file";
  }
  if (const char* env = std::getenv(EnvName(e.key.c_str()).c_str())) {
    if (Apply(e, env, true)) {
      e.source = "env";
    }
  }
  e.baseline = Serialise(e);
}

Entry& Register(const char* key, Kind kind) {
  auto entry = std::make_unique< Entry >();
  entry->key = key;
  entry->kind = kind;
  sEntries.push_back(std::move(entry));
  return *sEntries.back();
}

Entry* Find(const std::string& key) {
  for (auto& e : sEntries) {
    if (e->key == key) return e.get();
  }
  return nullptr;
}

bool SaveLocked() {
  if (!sDirty) return false;
  sDirty = false;
  if (sPath.empty()) return false;

  std::vector<std::string> lines = SplitLines(sOriginal);
  std::vector<std::string> appended;
  std::vector<Entry*> written;
  for (auto& up : sEntries) {
    Entry& e = *up;
    const std::string now = Serialise(e);
    if (now == e.baseline) continue; // untouched this run: whatever the file has stays
    const std::string line = e.key + "=" + now;
    bool replaced = false;
    for (size_t n = lines.size(); n-- > 0;) {
      std::string k, v;
      if (SplitLine(lines[n], k, v) && k == e.key) {
        lines[n] = line;
        replaced = true;
        break;
      }
    }
    if (!replaced) appended.push_back(line);
    written.push_back(&e);
  }
  std::string text;
  if (sOriginal.empty() && !appended.empty()) {
    text += "# Metroid Prime 2 port settings. An MP2_<KEY> environment variable overrides a key.\n";
  }
  for (const std::string& l : lines) text += l + "\n";
  for (const std::string& l : appended) text += l + "\n";
  if (text == sOriginal) return false; // identical: no write, no mtime touch

  const std::string tmp = sPath + ".tmp";
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    out << text;
    out.flush();
    if (!out) {
      std::fprintf(stderr, "[settings] cannot write %s\n", tmp.c_str());
      std::error_code ec;
      std::filesystem::remove(tmp, ec);
      return false;
    }
  }
  std::error_code ec;
  std::filesystem::rename(tmp, sPath, ec);
  if (ec) {
    std::fprintf(stderr, "[settings] cannot replace %s: %s\n", sPath.c_str(), ec.message().c_str());
    std::filesystem::remove(tmp, ec);
    return false;
  }
  sOriginal = text;
  for (Entry* e : written) {
    e->baseline = Serialise(*e);
    e->source = "file";
  }
  std::fprintf(stderr, "[settings] saved %s\n", sPath.c_str());
  return true;
}

} // namespace

void Initialize(const std::string& userDirectory) {
  std::lock_guard< std::mutex > lock(sMutex);
  sPath.clear();
  sOriginal.clear();
  sEntries.clear(); // a registry always belongs to one Initialize
  sDirty = false;
  if (userDirectory.empty()) return;
  sPath = (std::filesystem::path(userDirectory) / "settings.cfg").string();
  std::ifstream in(sPath, std::ios::binary);
  if (in) {
    std::ostringstream ss;
    ss << in.rdbuf();
    sOriginal = ss.str();
  }
}

void AddBool(const char* key, bool* value, bool defaultValue) {
  std::lock_guard< std::mutex > lock(sMutex);
  Entry& e = Register(key, Kind::Bool);
  e.b = value;
  *value = defaultValue;
  Resolve(e);
}

void AddInt(const char* key, int* value, int defaultValue, int min, int max,
            bool (*valid)(int), int zeroAs) {
  std::lock_guard< std::mutex > lock(sMutex);
  Entry& e = Register(key, Kind::Int);
  e.i = value;
  e.minI = min;
  e.maxI = max;
  e.valid = valid;
  e.zeroAs = zeroAs;
  *value = defaultValue;
  Resolve(e);
}

void AddFloat(const char* key, float* value, float defaultValue, float min, float max,
              bool allowZero) {
  std::lock_guard< std::mutex > lock(sMutex);
  Entry& e = Register(key, Kind::Float);
  e.f = value;
  e.minF = min;
  e.maxF = max;
  e.allowZero = allowZero;
  *value = defaultValue;
  Resolve(e);
}

void AddFloat3(const char* key, float value[3], float min, float max) {
  std::lock_guard< std::mutex > lock(sMutex);
  Entry& e = Register(key, Kind::Float3);
  e.f = value;
  e.minF = min;
  e.maxF = max;
  value[0] = value[1] = value[2] = 0.f;
  Resolve(e);
}

void NoteChanged() {
  std::lock_guard< std::mutex > lock(sMutex);
  sDirty = true;
}

bool Dirty() {
  std::lock_guard< std::mutex > lock(sMutex);
  return sDirty;
}

bool Save() {
  std::lock_guard< std::mutex > lock(sMutex);
  return SaveLocked();
}

bool BoolValue(const char* key, bool defaultValue) {
  std::lock_guard< std::mutex > lock(sMutex);
  bool result = defaultValue;
  std::string text;
  if (FileValue(key, text)) {
    ParseBool(text, false, result);
  }
  if (const char* env = std::getenv(EnvName(key).c_str())) {
    ParseBool(env, true, result);
  }
  return result;
}

std::vector<Info> List() {
  std::lock_guard< std::mutex > lock(sMutex);
  std::vector<Info> out;
  for (auto& e : sEntries) {
    out.push_back({e->key, Serialise(*e), e->source});
  }
  return out;
}

bool Set(const std::string& key, const std::string& value, std::string& error) {
  std::lock_guard< std::mutex > lock(sMutex);
  Entry* e = Find(key);
  if (e == nullptr) {
    error = "unknown setting '" + key + "' (settings lists them)";
    return false;
  }
  if (!Apply(*e, value, false)) {
    error = "'" + value + "' is not a valid value for " + key;
    return false;
  }
  sDirty = true;
  SaveLocked();
  return true;
}

} // namespace PortSettings
