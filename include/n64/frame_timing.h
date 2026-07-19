#pragma once

#include <cstdint>
#include <vector>

namespace n64 {

// The VR4300 in the N64 runs at roughly 93.75 MHz. We do not model
// cycle-accurate timing yet, so we simply divide the clock rate by the
// target frame rate to get a fixed instruction budget to retire per frame.
inline constexpr uint64_t kVr4300ClockHz = 93'750'000ULL;
inline constexpr int kTargetFps = 60;

// Number of CPU instructions the main loop should attempt to retire per frame
// to approximate real-time pacing. Pure arithmetic so it can be unit tested
// without a CPU or a window.
constexpr uint64_t InstructionsPerFrame(uint64_t clock_hz = kVr4300ClockHz, int fps = kTargetFps) {
  if (fps <= 0) {
    return 0;
  }
  return clock_hz / static_cast<uint64_t>(fps);
}

// Pixel dimensions of a frame, in a single struct so callers cannot silently
// transpose width and height.
struct FrameSize {
  int width = 0;
  int height = 0;
};

// Fills `pixels` with a simple animated diagonal gradient in RGBA8888 order.
// This is a placeholder visual used until the RDP/VI actually produce frames;
// `frame` advances the animation. `pixels` is resized to width*height*4 bytes.
void FillTestPattern(std::vector<uint8_t>& pixels, FrameSize size, uint64_t frame);

}  // namespace n64
