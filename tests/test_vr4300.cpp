#include "n64/rdram.h"
#include "n64/vr4300.h"
#include "test_framework.h"

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
CHECK(cpu.pc() == pc_before + 4);
TEST_MAIN_END()
