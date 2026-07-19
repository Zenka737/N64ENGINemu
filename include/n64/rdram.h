#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace n64 {

// Console RDRAM. Retail N64 units shipped with 4MB, expandable to 8MB via
// the Expansion Pak.
class RdRam {
 public:
  static constexpr size_t kBaseSize = 4ULL * 1024 * 1024;
  static constexpr size_t kExpandedSize = 8ULL * 1024 * 1024;

  explicit RdRam(size_t size = kBaseSize);

  uint8_t Read8(uint32_t address) const;
  void Write8(uint32_t address, uint8_t value);

  uint32_t Read32(uint32_t address) const;
  void Write32(uint32_t address, uint32_t value);

  size_t size() const { return data_.size(); }

 private:
  std::vector<uint8_t> data_;
};

}  // namespace n64
