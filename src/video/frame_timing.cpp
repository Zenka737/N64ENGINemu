#include "n64/frame_timing.h"

#include <cstddef>

namespace n64 {

void FillTestPattern(std::vector<uint8_t>& pixels, FrameSize size, uint64_t frame) {
  if (size.width <= 0 || size.height <= 0) {
    pixels.clear();
    return;
  }

  const size_t pixel_count = static_cast<size_t>(size.width) * static_cast<size_t>(size.height);
  pixels.resize(pixel_count * 4);

  const auto phase = static_cast<uint8_t>(frame);
  for (int y = 0; y < size.height; ++y) {
    for (int x = 0; x < size.width; ++x) {
      const size_t index =
          (static_cast<size_t>(y) * static_cast<size_t>(size.width) + static_cast<size_t>(x)) * 4;
      pixels[index + 0] = static_cast<uint8_t>(x + phase);            // R
      pixels[index + 1] = static_cast<uint8_t>(y + phase);            // G
      pixels[index + 2] = static_cast<uint8_t>((x + y) / 2 + phase);  // B
      pixels[index + 3] = 0xFF;                                       // A
    }
  }
}

}  // namespace n64
