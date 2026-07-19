#include "n64/vi.h"

#include <cstddef>

#include "n64/frame_timing.h"

namespace n64 {

void Vi::WriteRegister(uint32_t offset, uint32_t value) {
  const uint32_t index = offset / 4;
  // VI_CURRENT is read-only from software's point of view (the DAC drives it);
  // writes are used to acknowledge the vertical interrupt, not to set the line.
  if (index >= kRegisterCount || offset == kCurrent) {
    return;
  }
  regs_[offset / 4] = value;
}

uint32_t Vi::ReadRegister(uint32_t offset) const {
  const uint32_t index = offset / 4;
  if (index >= kRegisterCount) {
    return 0;
  }
  return regs_[index];
}

Vi::PixelFormat Vi::format() const { return static_cast<PixelFormat>(regs_[kStatus / 4] & 0x3); }

bool Vi::HasFramebuffer() const {
  const PixelFormat fmt = format();
  return (fmt == PixelFormat::kRgba5551 || fmt == PixelFormat::kRgba8888) && width() > 0;
}

uint32_t Vi::ExpandRgba5551(uint16_t pixel) {
  const auto r5 = static_cast<uint32_t>((pixel >> 11) & 0x1F);
  const auto g5 = static_cast<uint32_t>((pixel >> 6) & 0x1F);
  const auto b5 = static_cast<uint32_t>((pixel >> 1) & 0x1F);
  const uint32_t r = (r5 << 3) | (r5 >> 2);
  const uint32_t g = (g5 << 3) | (g5 >> 2);
  const uint32_t b = (b5 << 3) | (b5 >> 2);
  const uint32_t a = ((pixel & 0x1) != 0) ? 0xFF : 0x00;
  return (r << 24) | (g << 16) | (b << 8) | a;
}

std::vector<uint8_t> Vi::ReadFramebuffer(const RdRam& rdram) const {
  const uint32_t w = width();
  const uint32_t h = height();
  const PixelFormat fmt = format();
  const uint32_t base = origin();

  std::vector<uint8_t> out(static_cast<size_t>(w) * h * 4);
  const size_t bytes_per_pixel = (fmt == PixelFormat::kRgba8888) ? 4 : 2;

  for (uint32_t y = 0; y < h; ++y) {
    for (uint32_t x = 0; x < w; ++x) {
      const uint32_t addr = base + (y * w + x) * static_cast<uint32_t>(bytes_per_pixel);
      const size_t out_index = (static_cast<size_t>(y) * w + x) * 4;
      // Guard against a bogus origin/width running past RDRAM: emit black.
      if (static_cast<size_t>(addr) + bytes_per_pixel > rdram.size()) {
        out[out_index + 0] = 0;
        out[out_index + 1] = 0;
        out[out_index + 2] = 0;
        out[out_index + 3] = 0xFF;
        continue;
      }
      if (fmt == PixelFormat::kRgba8888) {
        const uint32_t p = rdram.Read32(addr);
        out[out_index + 0] = static_cast<uint8_t>(p >> 24);
        out[out_index + 1] = static_cast<uint8_t>(p >> 16);
        out[out_index + 2] = static_cast<uint8_t>(p >> 8);
        out[out_index + 3] = static_cast<uint8_t>(p);
      } else {
        const uint32_t p = ExpandRgba5551(rdram.Read16(addr));
        out[out_index + 0] = static_cast<uint8_t>(p >> 24);
        out[out_index + 1] = static_cast<uint8_t>(p >> 16);
        out[out_index + 2] = static_cast<uint8_t>(p >> 8);
        out[out_index + 3] = static_cast<uint8_t>(p);
      }
    }
  }
  return out;
}

std::vector<uint8_t> Vi::RenderFrameOrFallback(const RdRam& rdram, uint64_t frame,
                                               int fallback_width, int fallback_height) const {
  if (HasFramebuffer()) {
    return ReadFramebuffer(rdram);
  }
  std::vector<uint8_t> pixels;
  FillTestPattern(pixels, {fallback_width, fallback_height}, frame);
  return pixels;
}

}  // namespace n64
