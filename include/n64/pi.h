#pragma once

#include <cstdint>

#include "n64/rdram.h"
#include "n64/rom.h"

namespace n64 {

// Minimal model of the N64 Peripheral Interface's ROM -> RDRAM DMA engine.
//
// Real hardware exposes this as MMIO registers (PI_DRAM_ADDR/PI_CART_ADDR/
// PI_RD_LEN/PI_WR_LEN at physical 0x0460'0000+) that game code programs
// directly to stream cartridge data into RAM. Vr4300 doesn't dispatch MMIO
// yet (see the TODO in main.cpp), so game-triggered DMAs aren't supported.
// What *is* modeled here is the one DMA every retail cartridge depends on
// before it ever runs a single instruction: IPL3, the boot code baked into
// the first 0x1000 bytes of every ROM, copies the game's code/data (starting
// right after IPL3 itself) from the cartridge into RDRAM at a fixed address
// before jumping to the header's entry point. Without that transfer, the
// entry point refers to RDRAM that was never written, so the CPU "executes"
// zeroed memory (which happens to decode as SLL $0, $0, 0 -- a no-op) and
// walks straight off the end of RDRAM.
class Pi {
 public:
  // Cartridge ROM offset IPL3 copies from, and the RDRAM address it copies
  // to. See the n64dev "Booting the console" documentation.
  static constexpr uint32_t kBootRomOffset = 0x1000;
  static constexpr uint32_t kBootRdramAddress = 0x0000'0400;

  // Copies `length` bytes from `rom` starting at `rom_offset` into `rdram`
  // starting at `rdram_address`. Mirrors a single PI DMA transfer.
  // Throws std::out_of_range if either range doesn't fit in its buffer.
  static void Dma(const Rom& rom, RdRam& rdram, uint32_t rom_offset, uint32_t rdram_address,
                  uint32_t length);

  // Performs the ROM -> RDRAM transfer IPL3 performs before handing control
  // to the cartridge's entry point. Copies as much of the ROM (starting
  // after the header/bootcode) as fits into RDRAM starting at
  // kBootRdramAddress; does nothing if the ROM is smaller than
  // kBootRomOffset.
  static void BootCopy(const Rom& rom, RdRam& rdram);
};

}  // namespace n64
