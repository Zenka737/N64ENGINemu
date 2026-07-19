#include <cstdint>
#include <vector>

#include "n64/rdram.h"
#include "n64/vi.h"
#include "test_framework.h"

namespace {

// Builds a 16-bit RGBA5551 pixel from 5-bit channels and a 1-bit alpha.
uint16_t Pack5551(uint8_t r5, uint8_t g5, uint8_t b5, uint8_t a1) {
  return static_cast<uint16_t>((r5 << 11) | (g5 << 6) | (b5 << 1) | a1);
}

}  // namespace

TEST_MAIN_BEGIN()
// Register read/write round-trips at real VI offsets.
n64::Vi vi;
vi.WriteRegister(n64::Vi::kOrigin, 0x0010'0000);
vi.WriteRegister(n64::Vi::kWidth, 320);
vi.WriteRegister(n64::Vi::kStatus, 0x0000'0002);
CHECK(vi.ReadRegister(n64::Vi::kOrigin) == 0x0010'0000);
CHECK(vi.ReadRegister(n64::Vi::kWidth) == 320);
CHECK(vi.origin() == 0x0010'0000);
CHECK(vi.width() == 320);
CHECK(vi.height() == 240);  // 320 * 3 / 4
CHECK(vi.format() == n64::Vi::PixelFormat::kRgba5551);

// Out-of-range offsets are ignored on write and read as 0.
vi.WriteRegister(0x1000, 0xDEAD'BEEF);
CHECK(vi.ReadRegister(0x1000) == 0);

// VI_CURRENT is read-only (writes acknowledge interrupt, not set the line).
vi.WriteRegister(n64::Vi::kCurrent, 0x1234);
CHECK(vi.ReadRegister(n64::Vi::kCurrent) == 0);

// RGBA5551 -> RGBA8888 conversion of known colors (packed R<<24|G<<16|B<<8|A).
CHECK(n64::Vi::ExpandRgba5551(Pack5551(0x1F, 0, 0, 1)) == 0xFF0000FF);  // pure red, opaque
CHECK(n64::Vi::ExpandRgba5551(Pack5551(0, 0x1F, 0, 0)) == 0x00FF0000);  // pure green, transparent
CHECK(n64::Vi::ExpandRgba5551(Pack5551(0, 0, 0x1F, 1)) == 0x0000FFFF);  // pure blue, opaque
CHECK(n64::Vi::ExpandRgba5551(Pack5551(0x1F, 0x1F, 0x1F, 1)) == 0xFFFFFFFF);  // white
CHECK(n64::Vi::ExpandRgba5551(Pack5551(0, 0, 0, 0)) == 0x00000000);           // black
// Mid value 0x10 -> (0x10<<3)|(0x10>>2) = 0x80|0x04 = 0x84, not a bare 0x80.
CHECK((n64::Vi::ExpandRgba5551(Pack5551(0x10, 0, 0, 0)) >> 24) == 0x84);

// Blank/no-framebuffer fallback: status format 0 yields the test pattern at the
// requested fallback dimensions.
n64::RdRam rdram;
n64::Vi blank;
blank.WriteRegister(n64::Vi::kStatus, 0);  // kBlank
std::vector<uint8_t> fb = blank.RenderFrameOrFallback(rdram, 0, 8, 6);
CHECK(fb.size() == static_cast<size_t>(8 * 6 * 4));
CHECK(fb[3] == 0xFF);  // fallback alpha opaque

// End-to-end: poke a synthetic RGBA5551 framebuffer into RDRAM and read it.
const uint32_t kOrigin = 0x0010'0000;
n64::Vi fbvi;
fbvi.WriteRegister(n64::Vi::kOrigin, kOrigin);
fbvi.WriteRegister(n64::Vi::kWidth, 4);  // 4x3 framebuffer
fbvi.WriteRegister(n64::Vi::kStatus, static_cast<uint32_t>(n64::Vi::PixelFormat::kRgba5551));
CHECK(fbvi.width() == 4 && fbvi.height() == 3);
const uint16_t red = Pack5551(0x1F, 0, 0, 1);
for (uint32_t i = 0; i < 4 * 3; ++i) {
  rdram.Write16(kOrigin + i * 2, red);
}
std::vector<uint8_t> real = fbvi.RenderFrameOrFallback(rdram, 0, 640, 480);
CHECK(real.size() == static_cast<size_t>(4 * 3 * 4));
CHECK(real[0] == 0xFF && real[1] == 0x00 && real[2] == 0x00 && real[3] == 0xFF);
CHECK(real[real.size() - 4] == 0xFF);  // last pixel red too

// End-to-end RGBA8888 readout.
n64::Vi fbvi32;
fbvi32.WriteRegister(n64::Vi::kOrigin, kOrigin);
fbvi32.WriteRegister(n64::Vi::kWidth, 2);
fbvi32.WriteRegister(n64::Vi::kStatus, static_cast<uint32_t>(n64::Vi::PixelFormat::kRgba8888));
rdram.Write32(kOrigin, 0x11223344);
std::vector<uint8_t> real32 = fbvi32.RenderFrameOrFallback(rdram, 0, 640, 480);
CHECK(real32[0] == 0x11 && real32[1] == 0x22 && real32[2] == 0x33 && real32[3] == 0x44);
TEST_MAIN_END()
