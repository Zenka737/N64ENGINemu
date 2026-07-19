#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace n64 {

// Owns a window plus whatever presentation resources the platform backend
// needs to blit emulated frames to the screen. This is a thin presentation
// layer only: it knows nothing about the RDP/VI and simply uploads a
// caller-provided RGBA8888 image each frame. Windowing is intentionally kept
// out of the unit-tested core (headless CI has no display); the pure
// per-frame logic lives in frame_timing.h instead.
//
// On Windows this is backed by plain Win32 + GDI (no extra dependency to
// install). Elsewhere it is backed by SDL2.
class Video {
 public:
  Video(const std::string& title, int width, int height);
  ~Video();

  Video(const Video&) = delete;
  Video& operator=(const Video&) = delete;
  Video(Video&&) = delete;
  Video& operator=(Video&&) = delete;

  // Uploads `rgba_pixels` (width*height*4 bytes, RGBA8888) and presents it,
  // stretched to fill the window. A null or mismatched buffer is ignored.
  void PresentFrame(const uint8_t* rgba_pixels, int width, int height);

  // Pumps the platform event queue. Returns false once the user requests to
  // close the window (window close button or Escape key).
  bool PollEvents();

  struct Impl;

 private:
  std::unique_ptr<Impl> impl_;
};

}  // namespace n64
