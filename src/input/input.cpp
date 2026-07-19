#include "n64/input.h"

#include <cstdint>
#include <iostream>

#include "n64/controller.h"

namespace n64 {
namespace {

using Button = Controller::Button;

// Analog stick deflection applied when a stick key is held. The real hardware
// range is roughly +/-80 for a full tilt; we use that so the packed value looks
// like a genuine full deflection rather than the raw +/-127 extreme.
constexpr int8_t kStickFullTilt = 80;

}  // namespace

void PrintKeyBindings() {
  std::cout << "Controller key bindings:\n"
               "  Analog stick : W/A/S/D\n"
               "  D-Pad        : Arrow keys\n"
               "  A / B        : Z / X\n"
               "  Z trigger    : Left Shift\n"
               "  Start        : Enter\n"
               "  L / R        : Q / E\n"
               "  C buttons    : I/K/J/L (Up/Down/Left/Right)\n";
}

}  // namespace n64

#ifdef _WIN32

// clang-format off
#include <windows.h>
// clang-format on

namespace n64 {
namespace {

bool KeyDown(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

}  // namespace

void PollKeyboardInto(Controller& controller) {
  controller.Reset();

  controller.SetButton(Button::A, KeyDown('Z'));
  controller.SetButton(Button::B, KeyDown('X'));
  controller.SetButton(Button::Z, KeyDown(VK_LSHIFT));
  controller.SetButton(Button::Start, KeyDown(VK_RETURN));
  controller.SetButton(Button::L, KeyDown('Q'));
  controller.SetButton(Button::R, KeyDown('E'));

  controller.SetButton(Button::DUp, KeyDown(VK_UP));
  controller.SetButton(Button::DDown, KeyDown(VK_DOWN));
  controller.SetButton(Button::DLeft, KeyDown(VK_LEFT));
  controller.SetButton(Button::DRight, KeyDown(VK_RIGHT));

  controller.SetButton(Button::CUp, KeyDown('I'));
  controller.SetButton(Button::CDown, KeyDown('K'));
  controller.SetButton(Button::CLeft, KeyDown('J'));
  controller.SetButton(Button::CRight, KeyDown('L'));

  int x = 0;
  int y = 0;
  if (KeyDown('A')) {
    x -= kStickFullTilt;
  }
  if (KeyDown('D')) {
    x += kStickFullTilt;
  }
  if (KeyDown('S')) {
    y -= kStickFullTilt;
  }
  if (KeyDown('W')) {
    y += kStickFullTilt;
  }
  controller.SetStick(static_cast<int8_t>(x), static_cast<int8_t>(y));
}

}  // namespace n64

#else

#include <SDL.h>

namespace n64 {

void PollKeyboardInto(Controller& controller) {
  controller.Reset();

  const uint8_t* keys = SDL_GetKeyboardState(nullptr);
  if (keys == nullptr) {
    return;
  }

  const auto down = [keys](SDL_Scancode sc) { return keys[sc] != 0; };

  controller.SetButton(Button::A, down(SDL_SCANCODE_Z));
  controller.SetButton(Button::B, down(SDL_SCANCODE_X));
  controller.SetButton(Button::Z, down(SDL_SCANCODE_LSHIFT));
  controller.SetButton(Button::Start, down(SDL_SCANCODE_RETURN));
  controller.SetButton(Button::L, down(SDL_SCANCODE_Q));
  controller.SetButton(Button::R, down(SDL_SCANCODE_E));

  controller.SetButton(Button::DUp, down(SDL_SCANCODE_UP));
  controller.SetButton(Button::DDown, down(SDL_SCANCODE_DOWN));
  controller.SetButton(Button::DLeft, down(SDL_SCANCODE_LEFT));
  controller.SetButton(Button::DRight, down(SDL_SCANCODE_RIGHT));

  controller.SetButton(Button::CUp, down(SDL_SCANCODE_I));
  controller.SetButton(Button::CDown, down(SDL_SCANCODE_K));
  controller.SetButton(Button::CLeft, down(SDL_SCANCODE_J));
  controller.SetButton(Button::CRight, down(SDL_SCANCODE_L));

  int x = 0;
  int y = 0;
  if (down(SDL_SCANCODE_A)) {
    x -= kStickFullTilt;
  }
  if (down(SDL_SCANCODE_D)) {
    x += kStickFullTilt;
  }
  if (down(SDL_SCANCODE_S)) {
    y -= kStickFullTilt;
  }
  if (down(SDL_SCANCODE_W)) {
    y += kStickFullTilt;
  }
  controller.SetStick(static_cast<int8_t>(x), static_cast<int8_t>(y));
}

}  // namespace n64

#endif
