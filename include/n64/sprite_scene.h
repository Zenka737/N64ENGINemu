#pragma once

#include <cstdint>

#include "n64/controller.h"
#include "n64/rdram.h"

namespace n64 {

// A minimal hand-rolled "scene": a single colored square that moves around a
// fixed-size background, driven by Controller input. This is not real N64
// game code — it exists to demonstrate the whole loop working end to end
// (keyboard input -> Controller -> RDRAM -> VI/video) before the CPU can run
// a real game far enough to draw anything itself.
class SpriteScene {
 public:
  static constexpr uint32_t kWidth = 320;
  static constexpr uint32_t kHeight = 240;
  static constexpr uint32_t kSquareSize = 24;
  static constexpr int kSpeed = 4;

  struct Position {
    int x = static_cast<int>((kWidth - kSquareSize) / 2);
    int y = static_cast<int>((kHeight - kSquareSize) / 2);
  };

  // Moves the square based on D-Pad/analog-stick input, clamped so it always
  // stays fully on screen.
  void Advance(const Controller& pad);

  // Draws the background and the square into `rdram` at `origin` as RGBA5551,
  // matching Vi's expected framebuffer format.
  void Draw(RdRam& rdram, uint32_t origin) const;

  Position position() const { return position_; }

 private:
  Position position_;
};

}  // namespace n64
