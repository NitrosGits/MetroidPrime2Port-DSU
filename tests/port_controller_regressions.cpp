#include "port_input_capture.h"
#include "port_touch_activity.h"
#include "port_test.h"

using namespace PortInput;

int main() {
  PortTouch::Activity activity;
  REQUIRE(!activity.Active(true, false));
  activity.Touch();
  REQUIRE(activity.Active(true, false));
  REQUIRE(!activity.Active(false, false)); // Desktop without touch support.
  REQUIRE(!activity.Active(true, true)); // F1 does not impose touch's menu layout.
  activity.Physical();
  REQUIRE(!activity.Active(true, false));
  activity.Touch();
  REQUIRE(activity.Active(true, false)); // Switching back needs no settings change.

  SDL_Event event{};
  event.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
  REQUIRE(PortTouch::PhysicalEvent(event, false, false));
  REQUIRE(!PortTouch::PhysicalEvent(event, false, true));
  event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
  for (int value : {0, 200, 16000, -16000}) {
    event.gaxis.value = value;
    REQUIRE(!PortTouch::PhysicalEvent(event, false, false));
  }
  for (int value : {16001, -16001, 32767}) {
    event.gaxis.value = value;
    REQUIRE(PortTouch::PhysicalEvent(event, false, false));
    REQUIRE(!PortTouch::PhysicalEvent(event, false, true));
  }
  event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
  event.button.which = SDL_TOUCH_MOUSEID;
  REQUIRE(!PortTouch::PhysicalEvent(event, false, false));
  event.button.which = SDL_PEN_MOUSEID;
  REQUIRE(!PortTouch::PhysicalEvent(event, false, false));
  event.button.which = 0;
  REQUIRE(PortTouch::PhysicalEvent(event, false, false));
  event = {};
  event.type = SDL_EVENT_KEY_DOWN;
  event.key.scancode = SDL_SCANCODE_A;
  REQUIRE(PortTouch::PhysicalEvent(event, false, false));
  REQUIRE(!PortTouch::PhysicalEvent(event, true, false));
  event.key.repeat = true;
  REQUIRE(!PortTouch::PhysicalEvent(event, false, false));
  event.key.repeat = false;
  event.key.scancode = SDL_SCANCODE_VOLUMEUP;
  REQUIRE(!PortTouch::PhysicalEvent(event, false, false));

  // A full trigger pull crosses the synthetic half-pull and click thresholds
  // on different frames. None of those aliases may enter the recorded chord.
  std::vector<Input> chord;
  RawState raw;
  for (float pull : {0.51f, 0.61f, 1.f}) {
    raw.padAxes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] = pull;
    raw.padButtons = 1u << PAD_RAW_BUTTON_TRIGGER_RIGHT;
    if (pull == 1.f) raw.padButtons |= 1u << PAD_RAW_BUTTON_TRIGGER_RIGHT_CLICK;
    REQUIRE(!PortInputCapture::Accumulate(chord, PortInputCapture::DownInputs(raw)));
  }
  REQUIRE(chord.size() == 1);
  REQUIRE(chord[0].device == Device::PadAxis);
  REQUIRE(chord[0].code == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER && chord[0].dir == 1);
  REQUIRE(chord[0].threshold == 50);
  raw = {};
  REQUIRE(PortInputCapture::Accumulate(chord, PortInputCapture::DownInputs(raw)));

  chord.clear();
  raw.padButtons = (1u << PAD_RAW_BUTTON_TRIGGER_LEFT) | (1u << PAD_RAW_BUTTON_TRIGGER_LEFT_CLICK) |
                   (1u << PAD_RAW_BUTTON_TRIGGER_RIGHT) | (1u << PAD_RAW_BUTTON_TRIGGER_RIGHT_CLICK);
  raw.padAxes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] = raw.padAxes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] = 1.f;
  REQUIRE(!PortInputCapture::Accumulate(chord, PortInputCapture::DownInputs(raw)));
  REQUIRE(chord.size() == 2); // LT + RT is still a real chord.

  chord.clear();
  raw = {};
  raw.padButtons = 1u << SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
  REQUIRE(!PortInputCapture::Accumulate(chord, PortInputCapture::DownInputs(raw)));
  raw.padButtons |= (1u << PAD_RAW_BUTTON_TRIGGER_RIGHT) | (1u << PAD_RAW_BUTTON_TRIGGER_RIGHT_CLICK);
  raw.padAxes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] = 1.f;
  REQUIRE(!PortInputCapture::Accumulate(chord, PortInputCapture::DownInputs(raw)));
  REQUIRE(chord.size() == 2);
  REQUIRE(chord[0].device == Device::PadButton && chord[0].code == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
  REQUIRE(chord[1].device == Device::PadAxis && chord[1].code == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
  raw = {};
  REQUIRE(PortInputCapture::Accumulate(chord, PortInputCapture::DownInputs(raw)));

  raw.padAxes[SDL_GAMEPAD_AXIS_LEFTX] = -1.f;
  auto down = PortInputCapture::DownInputs(raw);
  REQUIRE(down.size() == 1 && down[0].dir == -1 && down[0].code == SDL_GAMEPAD_AXIS_LEFTX);
  raw = {};
  raw.keys.set(SDL_SCANCODE_A);
  raw.keys.set(SDL_SCANCODE_ESCAPE);
  raw.mouseButtons = 1;
  down = PortInputCapture::DownInputs(raw);
  REQUIRE(down.size() == 2 && down[0].device == Device::Key && down[1].device == Device::MouseButton);
  return 0;
}
