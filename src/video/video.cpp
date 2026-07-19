#include "n64/video.h"

#include <SDL.h>

#include <stdexcept>
#include <string>

namespace n64 {

Video::Video(const std::string& title, int width, int height) : width_(width), height_(height) {
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error(std::string("SDL video init failed: ") + SDL_GetError());
  }

  window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width,
                             height, SDL_WINDOW_SHOWN);
  if (window_ == nullptr) {
    const std::string error = SDL_GetError();
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    throw std::runtime_error("SDL window creation failed: " + error);
  }

  renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer_ == nullptr) {
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
  }
  if (renderer_ == nullptr) {
    const std::string error = SDL_GetError();
    SDL_DestroyWindow(window_);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    throw std::runtime_error("SDL renderer creation failed: " + error);
  }

  texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                               width, height);
  if (texture_ == nullptr) {
    const std::string error = SDL_GetError();
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    throw std::runtime_error("SDL texture creation failed: " + error);
  }
}

Video::~Video() {
  if (texture_ != nullptr) {
    SDL_DestroyTexture(texture_);
  }
  if (renderer_ != nullptr) {
    SDL_DestroyRenderer(renderer_);
  }
  if (window_ != nullptr) {
    SDL_DestroyWindow(window_);
  }
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void Video::PresentFrame(const uint8_t* rgba_pixels, int width, int height) {
  if (rgba_pixels == nullptr || width != width_ || height != height_) {
    return;
  }

  const int pitch = width_ * 4;
  SDL_UpdateTexture(texture_, nullptr, rgba_pixels, pitch);
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}

bool Video::PollEvents() {
  if (window_ == nullptr) {
    return false;
  }
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_QUIT) {
      return false;
    }
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
      return false;
    }
  }
  return true;
}

}  // namespace n64
