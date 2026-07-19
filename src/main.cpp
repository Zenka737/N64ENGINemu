#ifndef _WIN32
// Keep our own entry point instead of letting SDL rename main to SDL_main
// (which would require linking SDL2main and complicate the CLI build). Only
// needed on the SDL backend (non-Windows); Windows uses native Win32 video.
#define SDL_MAIN_HANDLED
#include <SDL.h>
#endif

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "n64/frame_timing.h"
#include "n64/rdram.h"
#include "n64/rom.h"
#include "n64/vi.h"
#include "n64/video.h"
#include "n64/vr4300.h"

#ifdef _WIN32
// clang-format off
#include <windows.h>
#include <commdlg.h>
// clang-format on

#include <optional>
#include <string>

namespace {

// Shows the native "Open File" dialog so users who launch the emulator
// without a ROM path (e.g. by double-clicking the .exe) can pick one.
std::optional<std::wstring> PromptForRomPath() {
  wchar_t file_path[MAX_PATH] = L"";

  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFilter = L"N64 ROMs (*.z64;*.n64;*.v64)\0*.z64;*.n64;*.v64\0All files\0*.*\0";
  ofn.lpstrFile = file_path;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrTitle = L"Select an N64 ROM";
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

  if (!GetOpenFileNameW(&ofn)) {
    return std::nullopt;
  }
  return std::wstring(file_path);
}

n64::Rom LoadRom(int argc, char** argv) {
  if (argc == 2) {
    return n64::Rom::LoadFromFile(argv[1]);
  }
  const std::optional<std::wstring> rom_path = PromptForRomPath();
  if (!rom_path) {
    throw std::runtime_error("No ROM selected.");
  }
  return n64::Rom::LoadFromFile(*rom_path);
}

}  // namespace
#else
namespace {

n64::Rom LoadRom(int argc, char** argv) {
  if (argc != 2) {
    throw std::runtime_error(std::string("Usage: ") + argv[0] + " <rom-path>");
  }
  return n64::Rom::LoadFromFile(argv[1]);
}

}  // namespace
#endif

namespace {

constexpr int kWindowWidth = 640;
constexpr int kWindowHeight = 480;

// Drives the CPU and presents frames until the window is closed. The CPU
// interpreter only implements a subset of the ISA, so Step() will throw a
// std::runtime_error on the first unimplemented opcode (which happens quickly
// on real game code). We catch that, stop stepping, and keep the window open
// showing the last frame so the user can see what happened.
void RunLoop(n64::Vr4300& cpu, n64::Video& video, n64::Vi& vi, const n64::RdRam& rdram) {
  const uint64_t instructions_per_frame = n64::InstructionsPerFrame();
  bool cpu_running = true;
  uint64_t frame = 0;

  while (video.PollEvents()) {
    if (cpu_running) {
      try {
        for (uint64_t i = 0; i < instructions_per_frame; ++i) {
          cpu.Step();
        }
      } catch (const std::exception& e) {
        std::cerr << "CPU stopped at pc=0x" << std::hex << cpu.pc() << std::dec << ": " << e.what()
                  << "\n";
        cpu_running = false;
      }
    }

    // The VI scans a real framebuffer out of RDRAM when one is configured,
    // otherwise it returns the animated test pattern. When a framebuffer is
    // configured the presented size follows VI_WIDTH/height; when blank it uses
    // the window size for the fallback.
    const bool has_fb = vi.format() == n64::Vi::PixelFormat::kRgba5551 ||
                        vi.format() == n64::Vi::PixelFormat::kRgba8888;
    const int present_w = has_fb ? static_cast<int>(vi.width()) : kWindowWidth;
    const int present_h = has_fb ? static_cast<int>(vi.height()) : kWindowHeight;
    const std::vector<uint8_t> framebuffer =
        vi.RenderFrameOrFallback(rdram, frame, kWindowWidth, kWindowHeight);
    video.PresentFrame(framebuffer.data(), present_w, present_h);
    ++frame;
  }
}

// Writes a small synthetic RGBA5551 checkerboard directly into RDRAM and points
// the VI at it. This is a demonstration hook: it proves the VI pixel pipeline
// reads and converts real RDRAM content, even though the CPU cannot yet program
// the VI itself (no MMIO dispatch exists in Vr4300 yet).
void PokeSyntheticFramebuffer(n64::RdRam& rdram, n64::Vi& vi) {
  constexpr uint32_t kOrigin = 0x0020'0000;  // 2MB into RDRAM, clear of low memory
  constexpr uint32_t kWidth = 320;
  const uint32_t height = kWidth * 3 / 4;
  const uint16_t red = 0xF801;    // RGBA5551: R=31, A=1
  const uint16_t green = 0x07C1;  // RGBA5551: G=31, A=1
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < kWidth; ++x) {
      const bool cell = (((x / 16) + (y / 16)) & 1) != 0;
      rdram.Write16(kOrigin + (y * kWidth + x) * 2, cell ? red : green);
    }
  }
  vi.WriteRegister(n64::Vi::kOrigin, kOrigin);
  vi.WriteRegister(n64::Vi::kWidth, kWidth);
  vi.WriteRegister(n64::Vi::kStatus, static_cast<uint32_t>(n64::Vi::PixelFormat::kRgba5551));
}

}  // namespace

int main(int argc, char** argv) {
#ifndef _WIN32
  SDL_SetMainReady();
#endif
  try {
    const n64::Rom rom = LoadRom(argc, argv);
    std::cout << "Loaded ROM: " << rom.header().image_name << " (" << rom.header().game_id << ")\n";
    std::cout << "Entry point: 0x" << std::hex << rom.header().entry_point << std::dec << "\n";

    n64::RdRam rdram;
    n64::Vr4300 cpu(rdram);
    cpu.Reset(rom.header().entry_point);

    // TODO(mmio): Vr4300::TranslateAddress routes every load/store straight to
    // RDRAM; it needs MMIO dispatch so CPU writes to the VI register range
    // (physical 0x0440'0000+) reach this Vi instead of raw RDRAM bytes. Until
    // then we program the VI here and poke a synthetic framebuffer to exercise
    // the real scanout path.
    n64::Vi vi;
    PokeSyntheticFramebuffer(rdram, vi);

    n64::Video video("N64ENGINemu - " + rom.header().image_name, kWindowWidth, kWindowHeight);
    RunLoop(cpu, video, vi, rdram);
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
