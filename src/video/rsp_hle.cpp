#include "n64/rsp_hle.h"

namespace n64 {

namespace {
constexpr uint8_t kOpEndDisplayList = 0xDF;
}  // namespace

RspHle::RspHle(RdRam& rdram, GfxBackend& gfx) : rdram_(rdram), gfx_(gfx) {}

void RspHle::RegisterCommand(uint8_t opcode, CommandHandler handler) {
  commands_[opcode] = std::move(handler);
}

void RspHle::RunDisplayList(uint32_t dl_address) {
  for (;;) {
    const uint64_t command =
        (static_cast<uint64_t>(rdram_.Read32(dl_address)) << 32) | rdram_.Read32(dl_address + 4);
    const auto opcode = static_cast<uint8_t>(command >> 56);

    if (opcode == kOpEndDisplayList) {
      return;
    }

    if (const auto it = commands_.find(opcode); it != commands_.end()) {
      it->second(*this, command);
    }
    // Unknown opcodes are skipped rather than treated as fatal: many
    // microcodes emit vendor-specific commands (matrices, lighting) that
    // don't need HLE handling to render correctly.

    dl_address += 8;
  }
}

}  // namespace n64
