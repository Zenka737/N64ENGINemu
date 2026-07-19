#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace n64 {

// Minimal model of VR4300 Coprocessor 0 (system control coprocessor).
//
// This is intentionally not a full implementation: it exists only to give
// MTC0/MFC0 somewhere honest to read and write, so that boot code that
// pokes Status/Config/Cause/Count/Compare etc. no longer hits an
// "unimplemented opcode" error. TLB, exception delivery, and interrupt
// semantics for these registers are NOT implemented here; register 12
// (Status), 13 (Cause), 9 (Count), and friends are just plain storage.
class Cop0 {
 public:
  static constexpr int kStatus = 12;
  static constexpr int kCause = 13;
  static constexpr int kCount = 9;
  static constexpr int kCompare = 11;

  uint32_t reg(int index) const { return regs_[static_cast<size_t>(index)]; }
  void set_reg(int index, uint32_t value) { regs_[static_cast<size_t>(index)] = value; }

 private:
  std::array<uint32_t, 32> regs_{};
};

}  // namespace n64
