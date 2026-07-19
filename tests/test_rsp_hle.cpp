#include "n64/rdram.h"
#include "n64/rsp_hle.h"
#include "test_framework.h"

namespace {

using n64::GfxBackend;
using n64::RdRam;
using n64::RspHle;

// Records the calls a display list produced instead of touching a real
// graphics API -- keeps the test fast and dependency-free.
class RecordingGfxBackend : public GfxBackend {
 public:
  int viewport_calls = 0;
  int triangle_calls = 0;

  void SetViewport(int, int, int, int) override { ++viewport_calls; }
  void BindTexture(uint32_t) override {}
  void DrawTriangle(const float*, const float*, const float*) override { ++triangle_calls; }
  void Present() override {}
};

constexpr uint8_t kOpSetViewport = 0x01;
constexpr uint8_t kOpDrawTriangle = 0x02;
constexpr uint8_t kOpEndDisplayList = 0xDF;

void WriteCommand(RdRam& rdram, uint32_t address, uint8_t opcode) {
  rdram.Write32(address, static_cast<uint32_t>(opcode) << 24);
  rdram.Write32(address + 4, 0);
}

}  // namespace

TEST_MAIN_BEGIN()

// Adding support for a new display-list command is exactly this: pick an
// opcode byte and register a handler. No RSP state machine to update.
{
  RdRam rdram;
  RecordingGfxBackend gfx;
  RspHle rsp(rdram, gfx);

  rsp.RegisterCommand(kOpSetViewport,
                      [](RspHle& r, uint64_t) { r.gfx().SetViewport(0, 0, 320, 240); });
  rsp.RegisterCommand(kOpDrawTriangle, [](RspHle& r, uint64_t) {
    const float v[3] = {0, 0, 0};
    r.gfx().DrawTriangle(v, v, v);
  });

  constexpr uint32_t kDlBase = 0x1000;
  WriteCommand(rdram, kDlBase, kOpSetViewport);
  WriteCommand(rdram, kDlBase + 8, kOpDrawTriangle);
  WriteCommand(rdram, kDlBase + 16, kOpDrawTriangle);
  WriteCommand(rdram, kDlBase + 24, kOpEndDisplayList);

  rsp.RunDisplayList(kDlBase);

  CHECK(gfx.viewport_calls == 1);
  CHECK(gfx.triangle_calls == 2);
}

// Unregistered opcodes are skipped rather than crashing the walk.
{
  RdRam rdram;
  RecordingGfxBackend gfx;
  RspHle rsp(rdram, gfx);

  constexpr uint32_t kDlBase = 0x2000;
  WriteCommand(rdram, kDlBase, 0xAA);  // Unknown vendor opcode.
  WriteCommand(rdram, kDlBase + 8, kOpEndDisplayList);

  rsp.RunDisplayList(kDlBase);

  CHECK(gfx.viewport_calls == 0);
  CHECK(gfx.triangle_calls == 0);
}

TEST_MAIN_END()
