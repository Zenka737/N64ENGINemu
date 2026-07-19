#pragma once

#include <cstdint>

namespace n64 {

// Models the state of a single standard N64 controller.
//
// The N64 controller reports its state over the joybus/PIF protocol as a
// 4-byte response: a 16-bit big-endian button bitfield followed by two signed
// bytes for the analog stick (X then Y). This class stores that exact layout so
// the packed value produced by Packed() is byte-for-byte what a game reads back.
//
// Button bit positions within the 16-bit field (bit 15 = MSB) follow the real
// hardware ordering documented on the n64brew wiki:
//
//   15 A        11 D-Up      7 Reset     3 C-Up
//   14 B        10 D-Down    6 (unused)  2 C-Down
//   13 Z         9 D-Left    5 L         1 C-Left
//   12 Start     8 D-Right   4 R         0 C-Right
class Controller {
 public:
  enum class Button : uint16_t {
    A = 1U << 15,
    B = 1U << 14,
    Z = 1U << 13,
    Start = 1U << 12,
    DUp = 1U << 11,
    DDown = 1U << 10,
    DLeft = 1U << 9,
    DRight = 1U << 8,
    Reset = 1U << 7,
    L = 1U << 5,
    R = 1U << 4,
    CUp = 1U << 3,
    CDown = 1U << 2,
    CLeft = 1U << 1,
    CRight = 1U << 0,
  };

  void SetButton(Button button, bool pressed);
  bool IsPressed(Button button) const;

  void SetStick(int8_t x, int8_t y);
  int8_t stick_x() const { return stick_x_; }
  int8_t stick_y() const { return stick_y_; }

  uint16_t buttons() const { return buttons_; }

  // Clears all buttons and recenters the stick.
  void Reset();

  // Packs the current state into the real 4-byte N64 controller response,
  // returned as a big-endian-ordered 32-bit value:
  //   [31..16] button bitfield, [15..8] signed stick X, [7..0] signed stick Y.
  uint32_t Packed() const;

 private:
  uint16_t buttons_ = 0;
  int8_t stick_x_ = 0;
  int8_t stick_y_ = 0;
};

}  // namespace n64
