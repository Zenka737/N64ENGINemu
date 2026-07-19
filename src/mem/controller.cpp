#include "n64/controller.h"

namespace n64 {

void Controller::SetButton(Button button, bool pressed) {
  const auto mask = static_cast<uint16_t>(button);
  if (pressed) {
    buttons_ |= mask;
  } else {
    buttons_ = static_cast<uint16_t>(buttons_ & ~mask);
  }
}

bool Controller::IsPressed(Button button) const {
  return (buttons_ & static_cast<uint16_t>(button)) != 0;
}

// x/y match the real hardware stick byte order and read more naturally than a
// wrapper struct would.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Controller::SetStick(int8_t x, int8_t y) {
  stick_x_ = x;
  stick_y_ = y;
}

void Controller::Reset() {
  buttons_ = 0;
  stick_x_ = 0;
  stick_y_ = 0;
}

uint32_t Controller::Packed() const {
  return (static_cast<uint32_t>(buttons_) << 16) |
         (static_cast<uint32_t>(static_cast<uint8_t>(stick_x_)) << 8) |
         static_cast<uint32_t>(static_cast<uint8_t>(stick_y_));
}

}  // namespace n64
