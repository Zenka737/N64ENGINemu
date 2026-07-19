#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "n64/rdram.h"
#include "n64/rom.h"
#include "n64/vr4300.h"

#ifdef _WIN32
#include <commdlg.h>
#include <windows.h>

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

int main(int argc, char** argv) {
  try {
    const n64::Rom rom = LoadRom(argc, argv);
    std::cout << "Loaded ROM: " << rom.header().image_name << " (" << rom.header().game_id << ")\n";
    std::cout << "Entry point: 0x" << std::hex << rom.header().entry_point << std::dec << "\n";

    n64::RdRam rdram;
    n64::Vr4300 cpu(rdram);
    cpu.Reset(rom.header().entry_point);
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
