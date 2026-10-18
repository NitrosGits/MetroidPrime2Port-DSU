#include "port_settings.h"

#include "port_env.h"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

namespace {

std::string sPath; // empty: no file

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

} // namespace

namespace PortSettings {

void Initialize(const std::string& userDirectory) {
  sPath.clear();
  if (userDirectory.empty())
    return;
  sPath = (std::filesystem::path(userDirectory) / "settings.cfg").string();
}

const std::string& FilePath() { return sPath; }

bool ReadFile(std::string& text) {
  if (sPath.empty())
    return false;
  std::ifstream in(sPath, std::ios::binary);
  if (!in.is_open())
    return false;
  text.assign(std::istreambuf_iterator< char >(in), std::istreambuf_iterator< char >());
  return true;
}

bool WriteFile(const std::string& text) {
  if (sPath.empty())
    return false;
  const std::string tmp = sPath + ".tmp";
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      std::fprintf(stderr, "port: could not write settings to %s\n", tmp.c_str());
      return false;
    }
    out << text;
    out.flush();
    if (!out) {
      std::fprintf(stderr, "port: could not write settings to %s (write failed)\n", tmp.c_str());
      return false;
    }
  }
  std::error_code ec;
  std::filesystem::rename(tmp, sPath, ec);
  if (ec) {
    std::fprintf(stderr, "port: could not replace %s: %s\n", sPath.c_str(), ec.message().c_str());
    std::filesystem::remove(tmp, ec);
    return false;
  }
  return true;
}

bool LogFileEnabled() {
  if (port::EnvString("MP2_LOG_FILE") != nullptr)
    return port::EnvFlag("MP2_LOG_FILE", true);
  std::string text;
  if (!ReadFile(text))
    return true;
  bool enabled = true;
  std::istringstream lines(text);
  std::string line;
  while (std::getline(lines, line)) {
    const size_t first = line.find_first_not_of(" \t");
    if (first == std::string::npos || line[first] == '#')
      continue;
    const size_t separator = line.find('=');
    if (separator == std::string::npos)
      continue;
    if (Trim(line.substr(0, separator)) == "log_file")
      enabled = ParseBool(Trim(line.substr(separator + 1)));
  }
  return enabled;
}

} // namespace PortSettings
