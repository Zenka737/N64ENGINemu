#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "n64/pi.h"
#include "n64/rdram.h"
#include "n64/rom.h"
#include "test_framework.h"

namespace {

// Builds a minimal big-endian (.z64) ROM image on disk: a valid header plus
// distinguishable bytes at and past Pi::kBootRomOffset so the DMA copy can be
// checked byte-for-byte.
std::string WriteTestRom() {
  std::vector<uint8_t> data(n64::Pi::kBootRomOffset + 0x100, 0);
  data[0] = 0x80;
  data[1] = 0x37;
  data[2] = 0x12;
  data[3] = 0x40;
  // Entry point (big-endian, offset 0x08): arbitrary but plausible KSEG0
  // address.
  data[0x08] = 0x80;
  data[0x09] = 0x00;
  data[0x0A] = 0x04;
  data[0x0B] = 0x00;
  // Fill the "game code" region (from kBootRomOffset onward) with
  // recognizable, non-zero bytes.
  for (size_t i = n64::Pi::kBootRomOffset; i < data.size(); ++i) {
    data[i] = static_cast<uint8_t>(0x10 + (i - n64::Pi::kBootRomOffset));
  }

  const std::string path =
      std::string(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp") + "/n64_pi_test.z64";
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
  file.close();
  return path;
}

}  // namespace

TEST_MAIN_BEGIN()
const std::string path = WriteTestRom();
const n64::Rom rom = n64::Rom::LoadFromFile(path);
std::remove(path.c_str());

// BootCopy should place the game code (everything from kBootRomOffset
// onward) at kBootRdramAddress in RDRAM.
{
  n64::RdRam rdram;
  n64::Pi::BootCopy(rom, rdram);

  const std::vector<uint8_t>& rom_data = rom.data();
  bool all_match = true;
  for (size_t i = n64::Pi::kBootRomOffset; i < rom_data.size(); ++i) {
    const uint32_t rdram_addr =
        n64::Pi::kBootRdramAddress + static_cast<uint32_t>(i - n64::Pi::kBootRomOffset);
    if (rdram.Read8(rdram_addr) != rom_data[i]) {
      all_match = false;
      break;
    }
  }
  CHECK(all_match);

  // Nothing should have been written before kBootRdramAddress.
  CHECK(rdram.Read8(0) == 0);
}

// Dma() copies an arbitrary range and rejects out-of-bounds requests.
{
  n64::RdRam rdram;
  n64::Pi::Dma(rom, rdram, n64::Pi::kBootRomOffset, 0x1000, 0x10);
  for (uint32_t i = 0; i < 0x10; ++i) {
    CHECK(rdram.Read8(0x1000 + i) == rom.data()[n64::Pi::kBootRomOffset + i]);
  }

  bool threw = false;
  try {
    n64::Pi::Dma(rom, rdram, static_cast<uint32_t>(rom.data().size()), 0, 4);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  CHECK(threw);

  threw = false;
  try {
    n64::Pi::Dma(rom, rdram, 0, static_cast<uint32_t>(rdram.size()), 4);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  CHECK(threw);
}
TEST_MAIN_END()
