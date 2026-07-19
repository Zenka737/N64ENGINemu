#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "n64/rdram.h"

namespace n64 {

// Models the N64 Video Interface (VI): the RCP block that scans a framebuffer
// out of RDRAM and drives the video DAC. Only the registers relevant to
// framebuffer scanout are modeled here; timing/sync registers are stored but
// otherwise inert. Register offsets and semantics are taken from the standard
// N64 memory map (VI base at physical 0x0440'0000 / KSEG1 0xA440'0000).
//
// NOTE: this class is not yet reachable by CPU load/store instructions. The
// VR4300 interpreter has no MMIO dispatch, so games cannot program these
// registers end-to-end yet (see the TODO in main.cpp). It is exercised directly
// from unit tests and main.cpp for now.
class Vi {
 public:
  // Byte offsets from the VI register base. These are real N64 offsets.
  static constexpr uint32_t kStatus = 0x00;   // pixel format / control (VI_CONTROL)
  static constexpr uint32_t kOrigin = 0x04;   // framebuffer physical address (VI_DRAM_ADDR)
  static constexpr uint32_t kWidth = 0x08;    // framebuffer width in pixels (VI_H_WIDTH)
  static constexpr uint32_t kIntr = 0x0C;     // vertical interrupt line
  static constexpr uint32_t kCurrent = 0x10;  // current half-line (VI_V_CURRENT_LINE)
  static constexpr uint32_t kBurst = 0x14;
  static constexpr uint32_t kVSync = 0x18;
  static constexpr uint32_t kHSync = 0x1C;
  static constexpr uint32_t kLeap = 0x20;
  static constexpr uint32_t kHStart = 0x24;
  static constexpr uint32_t kVStart = 0x28;
  static constexpr uint32_t kVBurst = 0x2C;
  static constexpr uint32_t kXScale = 0x30;
  static constexpr uint32_t kYScale = 0x34;
  static constexpr uint32_t kRegisterCount = 14;

  // VI_STATUS low two bits select the framebuffer pixel type. 0/1 mean no
  // valid framebuffer (blanked); 2 is 16-bit RGBA5551; 3 is 32-bit RGBA8888.
  enum class PixelFormat : uint32_t {
    kBlank = 0,
    kReserved = 1,
    kRgba5551 = 2,
    kRgba8888 = 3,
  };

  Vi() = default;

  void WriteRegister(uint32_t offset, uint32_t value);
  uint32_t ReadRegister(uint32_t offset) const;

  PixelFormat format() const;
  uint32_t origin() const { return regs_[kOrigin / 4]; }
  // VI_H_WIDTH is 12 bits of framebuffer width in pixels.
  uint32_t width() const { return regs_[kWidth / 4] & 0xFFF; }
  // Real N64 software derives visible height from the 4:3 aspect ratio, so a
  // 320-wide framebuffer is 240 tall. Height is not stored in a single VI
  // register (it falls out of X/Y scale and the active-video window), so we
  // approximate it as width * 3 / 4, which matches common N64 resolutions.
  uint32_t height() const { return width() * 3 / 4; }

  // Produces an RGBA8888 buffer for Video::PresentFrame. If a valid framebuffer
  // is configured, reads real pixels from `rdram` at VI_ORIGIN and converts.
  // Otherwise falls back to the animated test pattern so the window shows
  // something. `frame` only advances the fallback animation.
  std::vector<uint8_t> RenderFrameOrFallback(const RdRam& rdram, uint64_t frame, int fallback_width,
                                             int fallback_height) const;

  // Expands a 16-bit RGBA5551 pixel to a packed RGBA8888 value (R in the high
  // byte, A in the low byte). The 5-bit channels are expanded by replicating
  // their top bits (v << 3 | v >> 2) rather than a bare shift, so 0x1F maps to
  // 0xFF and the ramp stays even. The single alpha bit maps to 0x00 or 0xFF.
  static uint32_t ExpandRgba5551(uint16_t pixel);

 private:
  bool HasFramebuffer() const;

  std::vector<uint8_t> ReadFramebuffer(const RdRam& rdram) const;

  std::array<uint32_t, kRegisterCount> regs_ = {};
};

}  // namespace n64
