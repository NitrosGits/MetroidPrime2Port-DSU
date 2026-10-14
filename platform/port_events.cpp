// The game loop's per-iteration Aurora poll. It lives here rather than in
// src/MetroidPrime/main.cpp because aurora/event.h includes SDL3, which only the
// full build (mp_port_audio) has on its include path.

#include <aurora/aurora.h>
#include <aurora/event.h>
#include <dolphin/pad.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

struct ButtonName {
  const char* name;
  u16 mask;
};

constexpr ButtonName kButtons[] = {
    {"START", PAD_BUTTON_START}, {"A", PAD_BUTTON_A},         {"B", PAD_BUTTON_B},
    {"X", PAD_BUTTON_X},         {"Y", PAD_BUTTON_Y},         {"Z", PAD_TRIGGER_Z},
    {"L", PAD_TRIGGER_L},        {"R", PAD_TRIGGER_R},        {"UP", PAD_BUTTON_UP},
    {"DOWN", PAD_BUTTON_DOWN},   {"LEFT", PAD_BUTTON_LEFT},   {"RIGHT", PAD_BUTTON_RIGHT},
};

// MP2_INPUT="BUTTON@frame[+frames],...": hold BUTTON on pad 0 from that loop
// iteration for the given number of iterations (default 8), so boots can be
// driven past menus without a player. Iterations count PortPumpEvents calls.
void RunInputScript(unsigned frame) {
  static const char* script = std::getenv("MP2_INPUT");
  if (script == nullptr) {
    return;
  }
  u16 held = 0;
  bool active = false;
  for (const char* p = script; *p != '\0';) {
    const char* at = std::strchr(p, '@');
    if (at == nullptr) {
      break;
    }
    char* end = nullptr;
    const unsigned start = static_cast< unsigned >(std::strtoul(at + 1, &end, 10));
    unsigned length = 8;
    if (*end == '+') {
      length = static_cast< unsigned >(std::strtoul(end + 1, &end, 10));
    }
    for (const ButtonName& button : kButtons) {
      if (std::strlen(button.name) == static_cast< size_t >(at - p) &&
          std::strncmp(button.name, p, at - p) == 0) {
        if (frame >= start && frame < start + length) {
          held |= button.mask;
        }
        if (frame == start) {
          std::fprintf(stderr, "[input] %s at frame %u\n", button.name, frame);
        }
      }
    }
    active |= frame < start + length + 1;
    p = *end == ',' ? end + 1 : end;
  }
  if (!active) {
    static bool cleared = false;
    if (!cleared) {
      PADClearVirtualStatus(0);
      cleared = true;
    }
    return;
  }
  PADStatus status{};
  status.err = PAD_ERR_NONE;
  status.button = held;
  PADSetVirtualStatus(0, &status);
}

} // namespace

// Runs aurora_update (window, input and GX housekeeping); true once the window asks to quit.
bool PortPumpEvents() {
  static unsigned frame = 0;
  RunInputScript(frame++);
  for (const AuroraEvent* event = aurora_update(); event != nullptr && event->type != AURORA_NONE;
       ++event) {
    if (event->type == AURORA_EXIT) {
      return true;
    }
  }
  return false;
}
