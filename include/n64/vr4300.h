#pragma once

#include <array>
#include <cstdint>

#include "n64/cop0.h"
#include "n64/rdram.h"

namespace n64 {

// Interpreter core for the VR4300 (MIPS R4300i) CPU. Implements a
// scalar-integer subset of the MIPS I instruction set (ALU, branch/jump
// with delay slots, and byte/halfword/word loads and stores) sufficient
// to run straight-line and control-flow-heavy code, plus minimal
// Coprocessor 0 register move support (MTC0/MFC0) and CACHE-as-no-op. Full
// COP0 semantics (TLB, exceptions/interrupts), the FPU, and 64-bit MIPS III
// instructions are not implemented yet.
class Vr4300 {
 public:
  explicit Vr4300(RdRam& rdram);

  void Reset(uint32_t entry_point);

  uint64_t gpr(int index) const { return gpr_[static_cast<size_t>(index)]; }
  void set_gpr(int index, uint64_t value);

  uint32_t pc() const { return pc_; }

  const Cop0& cop0() const { return cop0_; }

  // Fetches, decodes, and executes exactly one instruction, honoring the
  // MIPS branch/jump delay slot (the instruction after a branch always
  // executes before control transfers to the target).
  void Step();

 private:
  static uint32_t TranslateAddress(uint32_t vaddr);

  uint32_t Fetch(uint32_t vaddr) const;
  void Execute(uint32_t instr);
  void ExecuteSpecial(uint32_t instr);
  void ExecuteRegImm(uint32_t instr);
  void ExecuteCop0(uint32_t instr);

  void Branch(bool condition, int32_t offset);
  void Jump(uint32_t target);

  RdRam& rdram_;
  Cop0 cop0_;
  std::array<uint64_t, 32> gpr_{};
  uint32_t pc_ = 0;
  uint32_t next_pc_ = 4;
};

}  // namespace n64
