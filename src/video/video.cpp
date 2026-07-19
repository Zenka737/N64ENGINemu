#include "n64/video.h"

#include <stdexcept>
#include <string>

#ifdef _WIN32

// clang-format off
#include <windows.h>
// clang-format on

#include <vector>

namespace n64 {

struct Video::Impl {
  HWND hwnd = nullptr;
  int width = 0;
  int height = 0;
  std::vector<uint8_t> bgra;
  bool quit = false;
};

namespace {

constexpr wchar_t kWindowClassName[] = L"N64ENGINemuWindow";

std::wstring Utf8ToWide(const std::string& text) {
  if (text.empty()) {
    return {};
  }
  const int size =
      MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
  std::wstring wide(static_cast<size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
  return wide;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  auto* impl = reinterpret_cast<Video::Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  switch (msg) {
    case WM_CREATE: {
      const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
      SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
      return 0;
    }
    case WM_CLOSE:
      if (impl != nullptr) {
        impl->quit = true;
      }
      return 0;
    case WM_KEYDOWN:
      if (impl != nullptr && wparam == VK_ESCAPE) {
        impl->quit = true;
      }
      return 0;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProcW(hwnd, msg, wparam, lparam);
  }
}

void RegisterWindowClassOnce() {
  static const bool kRegistered = [] {
    WNDCLASSW window_class{};
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.lpszClassName = kWindowClassName;
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));  // IDC_ARROW
    return RegisterClassW(&window_class) != 0;
  }();
  (void)kRegistered;
}

}  // namespace

Video::Video(const std::string& title, int width, int height) : impl_(std::make_unique<Impl>()) {
  impl_->width = width;
  impl_->height = height;
  RegisterWindowClassOnce();

  RECT rect{0, 0, width, height};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

  impl_->hwnd =
      CreateWindowExW(0, kWindowClassName, Utf8ToWide(title).c_str(), WS_OVERLAPPEDWINDOW,
                      CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
                      nullptr, nullptr, GetModuleHandleW(nullptr), impl_.get());
  if (impl_->hwnd == nullptr) {
    throw std::runtime_error("Win32 window creation failed");
  }
  ShowWindow(impl_->hwnd, SW_SHOW);
}

Video::~Video() {
  if (impl_->hwnd != nullptr) {
    DestroyWindow(impl_->hwnd);
  }
}

void Video::PresentFrame(const uint8_t* rgba_pixels, int width, int height) {
  if (rgba_pixels == nullptr || width != impl_->width || height != impl_->height) {
    return;
  }

  const auto pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
  impl_->bgra.resize(pixel_count * 4);
  for (size_t i = 0; i < pixel_count; ++i) {
    impl_->bgra[(i * 4) + 0] = rgba_pixels[(i * 4) + 2];
    impl_->bgra[(i * 4) + 1] = rgba_pixels[(i * 4) + 1];
    impl_->bgra[(i * 4) + 2] = rgba_pixels[(i * 4) + 0];
    impl_->bgra[(i * 4) + 3] = rgba_pixels[(i * 4) + 3];
  }

  BITMAPINFO bmi{};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height;  // Negative: top-down DIB.
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  RECT client{};
  GetClientRect(impl_->hwnd, &client);

  HDC hdc = GetDC(impl_->hwnd);
  StretchDIBits(hdc, 0, 0, client.right - client.left, client.bottom - client.top, 0, 0, width,
                height, impl_->bgra.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
  ReleaseDC(impl_->hwnd, hdc);
}

bool Video::PollEvents() {
  if (impl_->quit) {
    return false;
  }
  MSG msg;
  while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) != 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return !impl_->quit;
}

}  // namespace n64

#else

#include <SDL.h>

namespace n64 {

struct Video::Impl {
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  SDL_Texture* texture = nullptr;
  int width = 0;
  int height = 0;
};

Video::Video(const std::string& title, int width, int height) : impl_(std::make_unique<Impl>()) {
  impl_->width = width;
  impl_->height = height;

  if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error(std::string("SDL video init failed: ") + SDL_GetError());
  }

  impl_->window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   width, height, SDL_WINDOW_SHOWN);
  if (impl_->window == nullptr) {
    const std::string error = SDL_GetError();
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    throw std::runtime_error("SDL window creation failed: " + error);
  }

  impl_->renderer =
      SDL_CreateRenderer(impl_->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (impl_->renderer == nullptr) {
    impl_->renderer = SDL_CreateRenderer(impl_->window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (impl_->renderer == nullptr) {
    const std::string error = SDL_GetError();
    SDL_DestroyWindow(impl_->window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    throw std::runtime_error("SDL renderer creation failed: " + error);
  }

  impl_->texture = SDL_CreateTexture(impl_->renderer, SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_STREAMING, width, height);
  if (impl_->texture == nullptr) {
    const std::string error = SDL_GetError();
    SDL_DestroyRenderer(impl_->renderer);
    SDL_DestroyWindow(impl_->window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    throw std::runtime_error("SDL texture creation failed: " + error);
  }
}

Video::~Video() {
  if (impl_->texture != nullptr) {
    SDL_DestroyTexture(impl_->texture);
  }
  if (impl_->renderer != nullptr) {
    SDL_DestroyRenderer(impl_->renderer);
  }
  if (impl_->window != nullptr) {
    SDL_DestroyWindow(impl_->window);
  }
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void Video::PresentFrame(const uint8_t* rgba_pixels, int width, int height) {
  if (rgba_pixels == nullptr || width != impl_->width || height != impl_->height) {
    return;
  }

  const int pitch = impl_->width * 4;
  SDL_UpdateTexture(impl_->texture, nullptr, rgba_pixels, pitch);
  SDL_RenderClear(impl_->renderer);
  SDL_RenderCopy(impl_->renderer, impl_->texture, nullptr, nullptr);
  SDL_RenderPresent(impl_->renderer);
}

bool Video::PollEvents() {
  if (impl_->window == nullptr) {
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

#endif
