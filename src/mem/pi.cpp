#include "n64/pi.h"

#include <algorithm>
#include <stdexcept>

namespace n64 {

void Pi::Dma(const Rom& rom, RdRam& rdram, uint32_t rom_offset, uint32_t rdram_address,
             uint32_t length) {
  const std::vector<uint8_t>& data = rom.data();
  if (static_cast<uint64_t>(rom_offset) + length > data.size()) {
    throw std::out_of_range("PI DMA: ROM source range out of bounds");
  }
  if (static_cast<uint64_t>(rdram_address) + length > rdram.size()) {
    throw std::out_of_range("PI DMA: RDRAM destination range out of bounds");
  }

  for (uint32_t i = 0; i < length; ++i) {
    rdram.Write8(rdram_address + i, data[rom_offset + i]);
  }
}

void Pi::BootCopy(const Rom& rom, RdRam& rdram) {
  const std::vector<uint8_t>& data = rom.data();
  if (data.size() <= kBootRomOffset || rdram.size() <= kBootRdramAddress) {
    return;
  }

  const uint64_t rom_available = data.size() - kBootRomOffset;
  const uint64_t rdram_available = rdram.size() - kBootRdramAddress;
  const auto length = static_cast<uint32_t>(std::min(rom_available, rdram_available));

  Dma(rom, rdram, kBootRomOffset, kBootRdramAddress, length);
}

}  // namespace n64
