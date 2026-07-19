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

#include "n64/controller.h"
#include "n64/frame_timing.h"
#include "n64/input.h"
#include "n64/pi.h"
#include "n64/rdram.h"
#include "n64/rom.h"
#include "n64/sprite_scene.h"
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

constexpr uint32_t kSceneOrigin = 0x0020'0000;  // 2MB into RDRAM, clear of low memory.

// Drives the CPU, moves the on-screen square from input, and presents frames
// until the window is closed. The CPU interpreter only implements a subset of
// the ISA, so Step() will throw a std::runtime_error on the first
// unimplemented opcode (which happens quickly on real game code). We catch
// that, stop stepping, and keep the window open showing the scene so the user
// can still see (and move) something.
void RunLoop(n64::Vr4300& cpu, n64::Video& video, n64::Vi& vi, n64::RdRam& rdram) {
  const uint64_t instructions_per_frame = n64::InstructionsPerFrame();
  bool cpu_running = true;

  // Real keyboard input feeds a Controller each frame. The CPU can't run real
  // game code far enough to draw anything itself yet, so SpriteScene is a
  // stand-in "game": a square moved directly by the same input, drawn into
  // RDRAM and scanned out through the real VI pixel pipeline.
  n64::Controller pad;
  n64::SpriteScene scene;

  while (video.PollEvents()) {
    n64::PollKeyboardInto(pad);
    scene.Advance(pad);
    scene.Draw(rdram, kSceneOrigin);

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

    const std::vector<uint8_t> framebuffer =
        vi.RenderFrameOrFallback(rdram, 0, kWindowWidth, kWindowHeight);
    video.PresentFrame(framebuffer.data(), static_cast<int>(vi.width()),
                       static_cast<int>(vi.height()));
  }
}

// Points the VI at the SpriteScene's framebuffer. This is a deliberate
// simplification: the CPU cannot yet program the VI itself (no MMIO dispatch
// exists in Vr4300 — see the TODO in main() below), so the scene is driven
// directly from the emulator's own input-polling loop instead of by
// CPU-executed game code.
void ConfigureVi(n64::Vi& vi) {
  vi.WriteRegister(n64::Vi::kOrigin, kSceneOrigin);
  vi.WriteRegister(n64::Vi::kWidth, n64::SpriteScene::kWidth);
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
    // Real hardware relies on IPL3 (the cartridge's boot code) to DMA the
    // game's code/data from ROM into RDRAM before the CPU ever runs an
    // instruction at the header's entry point. Without this, entry_point
    // refers to RDRAM that was never written.
    n64::Pi::BootCopy(rom, rdram);

    n64::Vr4300 cpu(rdram);
    cpu.Reset(rom.header().entry_point);

    // TODO(mmio): Vr4300::TranslateAddress routes every load/store straight to
    // RDRAM; it needs MMIO dispatch so CPU writes to the VI register range
    // (physical 0x0440'0000+) reach this Vi instead of raw RDRAM bytes.
    n64::Vi vi;
    ConfigureVi(vi);

    n64::PrintKeyBindings();

    n64::Video video("N64ENGINemu - " + rom.header().image_name, kWindowWidth, kWindowHeight);
    RunLoop(cpu, video, vi, rdram);
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
