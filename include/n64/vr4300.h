#pragma once

#include <array>
#include <cstdint>

#include "n64/rdram.h"

namespace n64 {

// Minimal skeleton for the VR4300 (MIPS R4300i) CPU core. Instruction
// decoding/execution is not implemented yet; this only models register
// state and program counter so the surrounding system can be wired up.
class Vr4300 {
 public:
  explicit Vr4300(RdRam& rdram);

  void Reset(uint32_t entry_point);

  uint64_t gpr(int index) const { return gpr_[static_cast<size_t>(index)]; }
  void set_gpr(int index, uint64_t value);

  uint32_t pc() const { return pc_; }

  // Executes a single instruction. Currently a no-op placeholder that
  // advances the program counter.
  void Step();

 private:
  [[maybe_unused]] RdRam& rdram_;
  std::array<uint64_t, 32> gpr_{};
  uint32_t pc_ = 0;
};

}  // namespace n64
