#include <cstdint>

#include "n64/rdram.h"
#include "n64/vr4300.h"
#include "test_framework.h"

namespace {

constexpr uint32_t EncodeR(uint32_t rs, uint32_t rt, uint32_t rd, uint32_t shamt, uint32_t funct) {
  return (rs << 21) | (rt << 16) | (rd << 11) | (shamt << 6) | funct;
}

constexpr uint32_t EncodeI(uint32_t opcode, uint32_t rs, uint32_t rt, uint16_t imm) {
  return (opcode << 26) | (rs << 21) | (rt << 16) | imm;
}

constexpr uint32_t EncodeJ(uint32_t opcode, uint32_t target) {
  return (opcode << 26) | ((target >> 2) & 0x3FFFFFFu);
}

}  // namespace

TEST_MAIN_BEGIN()
n64::RdRam rdram;
n64::Vr4300 cpu(rdram);

cpu.Reset(0x80000400);
CHECK(cpu.pc() == 0x80000400);
for (int i = 0; i < 32; ++i) {
  CHECK(cpu.gpr(i) == 0);
}

cpu.set_gpr(0, 0x1234);
CHECK(cpu.gpr(0) == 0);  // r0 is hardwired to zero.

cpu.set_gpr(5, 0x1234);
CHECK(cpu.gpr(5) == 0x1234);

const uint32_t pc_before = cpu.pc();
cpu.Step();
CHECK(cpu.pc() == pc_before + 4);  // Zeroed memory decodes as SLL $0, $0, 0 (NOP).

// ADDI $1, $0, 5
rdram.Write32(0x400, EncodeI(0x08, 0, 1, 5));
cpu.Reset(0x80000400);
cpu.Step();
CHECK(cpu.gpr(1) == 5);
CHECK(cpu.pc() == 0x80000404);

// Branch with delay slot: BEQ $0, $0, 2 (always taken) followed by ADDI $2, $0, 1
// in the delay slot, landing on ADDI $3, $0, 7 at the branch target.
rdram.Write32(0x410, EncodeI(0x04, 0, 0, 2));
rdram.Write32(0x414, EncodeI(0x08, 0, 2, 1));
rdram.Write32(0x41C, EncodeI(0x08, 0, 3, 7));
cpu.Reset(0x80000410);
cpu.Step();
CHECK(cpu.pc() == 0x80000414);
CHECK(cpu.gpr(2) == 0);  // Delay slot has not executed yet.
cpu.Step();
CHECK(cpu.gpr(2) == 1);
CHECK(cpu.pc() == 0x8000041C);
cpu.Step();
CHECK(cpu.gpr(3) == 7);

// Store/load round trip: ADDI $5, $0, 1234; SW $5, 0x100($0); LW $6, 0x100($0).
rdram.Write32(0x500, EncodeI(0x08, 0, 5, 1234));
rdram.Write32(0x504, EncodeI(0x2B, 0, 5, 0x100));
rdram.Write32(0x508, EncodeI(0x23, 0, 6, 0x100));
cpu.Reset(0x80000500);
cpu.Step();
cpu.Step();
cpu.Step();
CHECK(cpu.gpr(6) == 1234);

// Jump with delay slot: J 0x80000700, delay slot ADDI $7, $0, 9,
// landing on ADDI $8, $0, 3 at the jump target.
rdram.Write32(0x600, EncodeJ(0x02, 0x80000700));
rdram.Write32(0x604, EncodeI(0x08, 0, 7, 9));
rdram.Write32(0x700, EncodeI(0x08, 0, 8, 3));
cpu.Reset(0x80000600);
cpu.Step();
CHECK(cpu.pc() == 0x80000604);
cpu.Step();
CHECK(cpu.gpr(7) == 9);
CHECK(cpu.pc() == 0x80000700);
cpu.Step();
CHECK(cpu.gpr(8) == 3);

// R-type ALU: ADD $3, $1, $2 with $1=5, $2=1 -> $3=6.
rdram.Write32(0x800, EncodeR(1, 2, 3, 0, 0x20));
cpu.Reset(0x80000800);
cpu.set_gpr(1, 5);
cpu.set_gpr(2, 1);
cpu.Step();
CHECK(cpu.gpr(3) == 6);

// COP0: MTC0 $4, Status(12); MTC0 $5, Cause(13); MTC0 $6, Count(9);
// then MFC0 back into other registers to verify the round trip.
constexpr uint32_t kCop0Opcode = 0x10;
constexpr uint32_t kMtc0SubOp = 0x04;
constexpr uint32_t kMfc0SubOp = 0x00;
rdram.Write32(0x900, EncodeR(kMtc0SubOp, 4, n64::Cop0::kStatus, 0, 0) | (kCop0Opcode << 26));
rdram.Write32(0x904, EncodeR(kMtc0SubOp, 5, n64::Cop0::kCause, 0, 0) | (kCop0Opcode << 26));
rdram.Write32(0x908, EncodeR(kMtc0SubOp, 6, n64::Cop0::kCount, 0, 0) | (kCop0Opcode << 26));
rdram.Write32(0x90C, EncodeR(kMfc0SubOp, 7, n64::Cop0::kStatus, 0, 0) | (kCop0Opcode << 26));
rdram.Write32(0x910, EncodeR(kMfc0SubOp, 8, n64::Cop0::kCause, 0, 0) | (kCop0Opcode << 26));
rdram.Write32(0x914, EncodeR(kMfc0SubOp, 9, n64::Cop0::kCount, 0, 0) | (kCop0Opcode << 26));
cpu.Reset(0x80000900);
cpu.set_gpr(4, 0x34000000);  // Typical boot-time Status value.
cpu.set_gpr(5, 0x0000007C);
cpu.set_gpr(6, 0x12345678);
cpu.Step();  // MTC0 Status
cpu.Step();  // MTC0 Cause
cpu.Step();  // MTC0 Count
CHECK(cpu.cop0().reg(n64::Cop0::kStatus) == 0x34000000);
CHECK(cpu.cop0().reg(n64::Cop0::kCause) == 0x0000007C);
CHECK(cpu.cop0().reg(n64::Cop0::kCount) == 0x12345678);
cpu.Step();  // MFC0 $7, Status
CHECK(cpu.gpr(7) == 0x34000000);
cpu.Step();  // MFC0 $8, Cause
CHECK(cpu.gpr(8) == 0x0000007C);
cpu.Step();  // MFC0 $9, Count
CHECK(cpu.gpr(9) == 0x12345678);
CHECK(cpu.pc() == 0x80000918);

// CACHE is a no-op: it must not alter any GPR or COP0 register, only PC.
rdram.Write32(0xA00, EncodeI(0x2F, 0, 0, 0));  // CACHE (fields otherwise ignored).
cpu.Reset(0x80000A00);
cpu.set_gpr(1, 0xDEADBEEF);
const uint32_t status_before = cpu.cop0().reg(n64::Cop0::kStatus);
cpu.Step();
CHECK(cpu.pc() == 0x80000A04);
CHECK(cpu.gpr(1) == 0xDEADBEEF);
CHECK(cpu.cop0().reg(n64::Cop0::kStatus) == status_before);
TEST_MAIN_END()
