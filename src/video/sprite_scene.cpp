#include "n64/sprite_scene.h"

#include <algorithm>

namespace n64 {

namespace {

constexpr uint16_t kBackgroundColor = 0x0001;  // RGBA5551: black, alpha=1.
constexpr uint16_t kSquareColor = 0xFFFF;      // RGBA5551: white, alpha=1.
constexpr int8_t kStickThreshold = 32;

}  // namespace

void SpriteScene::Advance(const Controller& pad) {
  int dx = 0;
  int dy = 0;

  if (pad.IsPressed(Controller::Button::DLeft) || pad.stick_x() < -kStickThreshold) {
    dx -= kSpeed;
  }
  if (pad.IsPressed(Controller::Button::DRight) || pad.stick_x() > kStickThreshold) {
    dx += kSpeed;
  }
  if (pad.IsPressed(Controller::Button::DUp) || pad.stick_y() > kStickThreshold) {
    dy -= kSpeed;
  }
  if (pad.IsPressed(Controller::Button::DDown) || pad.stick_y() < -kStickThreshold) {
    dy += kSpeed;
  }

  position_.x = std::clamp(position_.x + dx, 0, static_cast<int>(kWidth - kSquareSize));
  position_.y = std::clamp(position_.y + dy, 0, static_cast<int>(kHeight - kSquareSize));
}

void SpriteScene::Draw(RdRam& rdram, uint32_t origin) const {
  for (uint32_t y = 0; y < kHeight; ++y) {
    const bool row_in_square = static_cast<int>(y) >= position_.y &&
                               static_cast<int>(y) < position_.y + static_cast<int>(kSquareSize);
    for (uint32_t x = 0; x < kWidth; ++x) {
      const bool col_in_square = static_cast<int>(x) >= position_.x &&
                                 static_cast<int>(x) < position_.x + static_cast<int>(kSquareSize);
      const uint16_t pixel = (row_in_square && col_in_square) ? kSquareColor : kBackgroundColor;
      rdram.Write16(origin + ((y * kWidth) + x) * 2, pixel);
    }
  }
}

}  // namespace n64
