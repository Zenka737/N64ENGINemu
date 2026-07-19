#pragma once

#include <cstdint>
#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace n64 {

// Owns an SDL window plus the renderer/texture used to blit emulated frames to
// the screen. This is a thin presentation layer only: it knows nothing about
// the RDP/VI and simply uploads a caller-provided RGBA8888 image each frame.
// Windowing is intentionally kept out of the unit-tested core (headless CI has
// no display); the pure per-frame logic lives in frame_timing.h instead.
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

  // Pumps the SDL event queue. Returns false once the user requests to close
  // the window (window close button or Escape key).
  bool PollEvents();

 private:
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* texture_ = nullptr;
  int width_ = 0;
  int height_ = 0;
};

}  // namespace n64
