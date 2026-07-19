#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace n64 {

// Minimal model of VR4300 Coprocessor 1 (the FPU).
//
// This is intentionally not a full implementation: it exists only to give
// MTC1/MFC1/CFC1/CTC1 somewhere honest to read and write, so that boot code
// that pokes the FPU control/status register (FCR31, typically to set the
// rounding mode) no longer hits an "unimplemented opcode" error. Actual
// floating-point arithmetic (ADD.S/MUL.D/...), conditional FPU branches
// (BC1T/BC1F), and FPU exception semantics are NOT implemented here; the 32
// FPU registers and FCR31 are just plain storage.
class Cop1 {
 public:
  static constexpr int kFcrControlStatus = 31;

  uint32_t reg(int index) const { return regs_[static_cast<size_t>(index)]; }
  void set_reg(int index, uint32_t value) { regs_[static_cast<size_t>(index)] = value; }

  uint32_t fcr(int index) const { return fcr_[static_cast<size_t>(index)]; }
  void set_fcr(int index, uint32_t value) { fcr_[static_cast<size_t>(index)] = value; }

 private:
  // FR=0 (32-bit) view: 32 independent 32-bit registers. The real VR4300
  // also supports a 64-bit FR=1 mode where even/odd register pairs combine
  // into doubles, but that distinction doesn't matter for plain register
  // moves, so it is left for a future, more complete FPU implementation.
  std::array<uint32_t, 32> regs_{};
  std::array<uint32_t, 32> fcr_{};
};

}  // namespace n64
