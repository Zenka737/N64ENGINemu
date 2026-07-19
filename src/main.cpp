// Keep our own entry point instead of letting SDL rename main to SDL_main
// (which would require linking SDL2main and complicate the MSVC/CLI build).
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "n64/frame_timing.h"
#include "n64/rdram.h"
#include "n64/rom.h"
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
void RunLoop(n64::Vr4300& cpu, n64::Video& video) {
  const uint64_t instructions_per_frame = n64::InstructionsPerFrame();
  std::vector<uint8_t> framebuffer;
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

    n64::FillTestPattern(framebuffer, {kWindowWidth, kWindowHeight}, frame);
    video.PresentFrame(framebuffer.data(), kWindowWidth, kWindowHeight);
    ++frame;
  }
}

}  // namespace

int main(int argc, char** argv) {
  SDL_SetMainReady();
  try {
    const n64::Rom rom = LoadRom(argc, argv);
    std::cout << "Loaded ROM: " << rom.header().image_name << " (" << rom.header().game_id << ")\n";
    std::cout << "Entry point: 0x" << std::hex << rom.header().entry_point << std::dec << "\n";

    n64::RdRam rdram;
    n64::Vr4300 cpu(rdram);
    cpu.Reset(rom.header().entry_point);

    n64::Video video("N64ENGINemu - " + rom.header().image_name, kWindowWidth, kWindowHeight);
    RunLoop(cpu, video);
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
