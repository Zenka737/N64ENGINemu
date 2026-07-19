#include "n64/rdram.h"
#include "test_framework.h"

TEST_MAIN_BEGIN()
n64::RdRam rdram;

CHECK(rdram.size() == n64::RdRam::kBaseSize);

rdram.Write8(0x10, 0xAB);
CHECK(rdram.Read8(0x10) == 0xAB);

rdram.Write32(0x100, 0xDEADBEEF);
CHECK(rdram.Read32(0x100) == 0xDEADBEEF);
CHECK(rdram.Read8(0x100) == 0xDE);
CHECK(rdram.Read8(0x103) == 0xEF);

bool threw = false;
try {
  rdram.Read8(static_cast<uint32_t>(rdram.size()));
} catch (const std::out_of_range&) {
  threw = true;
}
CHECK(threw);
TEST_MAIN_END()
