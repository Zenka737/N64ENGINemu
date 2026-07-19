#include <cstdlib>
#include <iostream>

#include "n64/rdram.h"
#include "n64/rom.h"
#include "n64/vr4300.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <rom-path>\n";
    return EXIT_FAILURE;
  }

  try {
    const n64::Rom rom = n64::Rom::LoadFromFile(argv[1]);
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
