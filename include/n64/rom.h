#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace n64 {

enum class RomFormat {
  kZ64,  // big-endian, native
  kN64,  // byte-swapped (little-endian)
  kV64,  // half-word swapped
  kUnknown,
};

struct RomHeader {
  uint32_t entry_point = 0;
  std::string image_name;
  std::string game_id;
};

class Rom {
 public:
  // Loads a ROM image from disk, normalizing it to big-endian (.z64) order.
  // Throws std::runtime_error on I/O failure or an unrecognized ROM format.
  static Rom LoadFromFile(const std::string& path);

  const RomHeader& header() const { return header_; }
  const std::vector<uint8_t>& data() const { return data_; }
  RomFormat format() const { return format_; }

 private:
  static RomFormat DetectFormat(const std::vector<uint8_t>& raw);
  static void NormalizeToBigEndian(std::vector<uint8_t>& data, RomFormat format);
  static RomHeader ParseHeader(const std::vector<uint8_t>& data);

  std::vector<uint8_t> data_;
  RomHeader header_;
  RomFormat format_ = RomFormat::kUnknown;
};

}  // namespace n64
