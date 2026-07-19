#include "n64/vr4300.h"

namespace n64 {

Vr4300::Vr4300(RdRam& rdram) : rdram_(rdram) {}

void Vr4300::Reset(uint32_t entry_point) {
  gpr_.fill(0);
  pc_ = entry_point;
}

void Vr4300::set_gpr(int index, uint64_t value) {
  if (index == 0) {
    return;  // r0 is hardwired to zero.
  }
  gpr_[static_cast<size_t>(index)] = value;
}

void Vr4300::Step() {
  // Instruction fetch/decode/execute is not implemented yet.
  pc_ += 4;
}

}  // namespace n64
