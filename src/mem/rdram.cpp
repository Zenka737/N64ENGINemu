#include "n64/rdram.h"

#include <stdexcept>

namespace n64 {

RdRam::RdRam(size_t size) : data_(size, 0) {}

uint8_t RdRam::Read8(uint32_t address) const {
  if (address >= data_.size()) {
    throw std::out_of_range("RDRAM read out of range");
  }
  return data_[address];
}

void RdRam::Write8(uint32_t address, uint8_t value) {
  if (address >= data_.size()) {
    throw std::out_of_range("RDRAM write out of range");
  }
  data_[address] = value;
}

uint32_t RdRam::Read32(uint32_t address) const {
  if (address + 4 > data_.size()) {
    throw std::out_of_range("RDRAM read out of range");
  }
  return (static_cast<uint32_t>(data_[address]) << 24) |
         (static_cast<uint32_t>(data_[address + 1]) << 16) |
         (static_cast<uint32_t>(data_[address + 2]) << 8) |
         static_cast<uint32_t>(data_[address + 3]);
}

void RdRam::Write32(uint32_t address, uint32_t value) {
  if (address + 4 > data_.size()) {
    throw std::out_of_range("RDRAM write out of range");
  }
  data_[address] = static_cast<uint8_t>(value >> 24);
  data_[address + 1] = static_cast<uint8_t>(value >> 16);
  data_[address + 2] = static_cast<uint8_t>(value >> 8);
  data_[address + 3] = static_cast<uint8_t>(value);
}

}  // namespace n64
