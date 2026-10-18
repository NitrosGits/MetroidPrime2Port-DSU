// The settings file's rules: env beats file beats default, bad values are ignored, and a run that
// changes nothing never writes (or creates) the file.

#include "port_settings.h"

#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

int gFailures = 0;

void Check(bool condition, const char* message) {
  if (!condition) {
    std::printf("FAIL: %s\n", message);
    ++gFailures;
  }
}

std::string Slurp(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

void Write(const std::string& path, const std::string& text) {
  std::ofstream(path, std::ios::binary | std::ios::trunc) << text;
}

bool MsaaValid(int v) { return v == 1 || v == 4; }

struct Vars {
  bool fast = false, spring = false;
  int msaa = 1, hud = 100;
  float fov = 0.f;
  float gun[3] = {};
};

void Register(Vars& v) {
  PortSettings::AddBool("fast_morph", &v.fast, false);
  PortSettings::AddBool("spring_ball", &v.spring, false);
  PortSettings::AddInt("msaa", &v.msaa, 1, 1, 4, MsaaValid);
  PortSettings::AddInt("hud_scale", &v.hud, 100, 50, 100, nullptr, 100);
  PortSettings::AddFloat("fov", &v.fov, 0.f, 30.f, 120.f, true);
  PortSettings::AddFloat3("gun_offset", v.gun, -1.f, 1.f);
}

} // namespace

int main() {
  const std::string dir = (std::filesystem::temp_directory_path() / "port_settings_test").string();
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  const std::string file = dir + "/settings.cfg";
  unsetenv("MP2_SPRING_BALL");

  { // a run that changes nothing: no file
    Vars v;
    PortSettings::Initialize(dir);
    Register(v);
    Check(!PortSettings::Save(), "clean save writes nothing");
    PortSettings::NoteChanged();
    Check(!PortSettings::Save(), "dirty but unchanged writes nothing");
    Check(!std::filesystem::exists(file), "no file created");
  }
  { // a change creates it
    Vars v;
    PortSettings::Initialize(dir);
    Register(v);
    v.fast = true;
    PortSettings::NoteChanged();
    Check(PortSettings::Save(), "change saves");
    Check(Slurp(file).find("fast_morph=1") != std::string::npos, "file has fast_morph=1");
  }
  // a hand-written file: comment, unknown key, bad values
  Write(file, "# mine\nfast_morph=1\nfuture_key=7\nhud_scale=999\nmsaa=2\nfov=10\n");
  struct stat before;
  stat(file.c_str(), &before);
  const std::string text = Slurp(file);
  {
    Vars v;
    PortSettings::Initialize(dir);
    Register(v);
    Check(v.fast, "file value read");
    Check(v.hud == 100 && v.msaa == 1 && v.fov == 0.f, "out-of-range values ignored");
    PortSettings::NoteChanged();
    Check(!PortSettings::Save(), "unchanged run does not write");
    Check(Slurp(file) == text, "file byte-identical");
    setenv("MP2_SPRING_BALL", "1", 1);
  }
  {
    Vars v;
    PortSettings::Initialize(dir);
    Register(v);
    Check(v.spring, "env wins");
    bool seenEnv = false;
    for (auto& i : PortSettings::List()) seenEnv |= i.key == "spring_ball" && std::string(i.source) == "env";
    Check(seenEnv, "source reported as env");
    v.msaa = 4;
    PortSettings::NoteChanged();
    Check(PortSettings::Save(), "change saves");
    const std::string now = Slurp(file);
    Check(now.find("# mine") != std::string::npos && now.find("future_key=7") != std::string::npos,
          "comments and unknown keys kept");
    Check(now.find("spring_ball") == std::string::npos, "env value not persisted");
    Check(now.find("msaa=4") != std::string::npos && now.find("msaa=2") == std::string::npos,
          "changed key rewritten in place");
    Check(now.find("hud_scale=999") != std::string::npos, "untouched invalid line kept");
    std::string err;
    Check(!PortSettings::Set("nope", "1", err), "unknown key refused");
    Check(!PortSettings::Set("msaa", "3", err), "invalid value refused");
    Check(PortSettings::Set("hud_scale", "0", err), "0 means max");
  }
  std::filesystem::remove_all(dir);
  std::printf(gFailures == 0 ? "port_settings_tests: ok\n" : "port_settings_tests: FAILED\n");
  return gFailures == 0 ? 0 : 1;
}
