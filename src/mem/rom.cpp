#include "n64/rom.h"

#include <fstream>
#include <stdexcept>

namespace n64 {

namespace {

uint32_t ReadBigEndian32(const std::vector<uint8_t>& data, size_t offset) {
  return (static_cast<uint32_t>(data[offset]) << 24) |
         (static_cast<uint32_t>(data[offset + 1]) << 16) |
         (static_cast<uint32_t>(data[offset + 2]) << 8) | static_cast<uint32_t>(data[offset + 3]);
}

}  // namespace

RomFormat Rom::DetectFormat(const std::vector<uint8_t>& raw) {
  if (raw.size() < 4) {
    return RomFormat::kUnknown;
  }
  if (raw[0] == 0x80 && raw[1] == 0x37 && raw[2] == 0x12 && raw[3] == 0x40) {
    return RomFormat::kZ64;
  }
  if (raw[0] == 0x37 && raw[1] == 0x80 && raw[2] == 0x40 && raw[3] == 0x12) {
    return RomFormat::kV64;
  }
  if (raw[0] == 0x40 && raw[1] == 0x12 && raw[2] == 0x37 && raw[3] == 0x80) {
    return RomFormat::kN64;
  }
  return RomFormat::kUnknown;
}

void Rom::NormalizeToBigEndian(std::vector<uint8_t>& data, RomFormat format) {
  const size_t aligned_size = data.size() - (data.size() % 4);
  switch (format) {
    case RomFormat::kZ64:
      return;
    case RomFormat::kV64:
      for (size_t i = 0; i < aligned_size; i += 2) {
        std::swap(data[i], data[i + 1]);
      }
      return;
    case RomFormat::kN64:
      for (size_t i = 0; i < aligned_size; i += 4) {
        std::swap(data[i], data[i + 3]);
        std::swap(data[i + 1], data[i + 2]);
      }
      return;
    case RomFormat::kUnknown:
      throw std::runtime_error("Unrecognized N64 ROM format");
  }
}

RomHeader Rom::ParseHeader(const std::vector<uint8_t>& data) {
  if (data.size() < 0x40) {
    throw std::runtime_error("ROM too small to contain a valid header");
  }
  RomHeader header;
  header.entry_point = ReadBigEndian32(data, 0x08);
  header.image_name.assign(reinterpret_cast<const char*>(&data[0x20]), 20);
  while (!header.image_name.empty() &&
         (header.image_name.back() == '\0' || header.image_name.back() == ' ')) {
    header.image_name.pop_back();
  }
  header.game_id.assign(reinterpret_cast<const char*>(&data[0x3B]), 4);
  return header;
}

Rom Rom::LoadFromFile(const std::string& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    throw std::runtime_error("Failed to open ROM file: " + path);
  }

  const std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> raw(static_cast<size_t>(size));
  if (!file.read(reinterpret_cast<char*>(raw.data()), size)) {
    throw std::runtime_error("Failed to read ROM file: " + path);
  }

  Rom rom;
  rom.format_ = DetectFormat(raw);
  NormalizeToBigEndian(raw, rom.format_);
  rom.header_ = ParseHeader(raw);
  rom.data_ = std::move(raw);
  return rom;
}

}  // namespace n64
