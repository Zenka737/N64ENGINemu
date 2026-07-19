#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>

#include "n64/gfx_backend.h"
#include "n64/rdram.h"

namespace n64 {

// High-level RSP emulation. Real hardware runs a signal processor that
// interprets a game-supplied microcode (F3DEX, S2DEX, ...) to build
// triangles and audio samples. We skip simulating that processor entirely:
// a game's display list is a sequence of 8-byte commands, each tagged by
// its top byte with an opcode. We read that opcode directly and call the
// matching handler, which issues the equivalent GfxBackend calls. No
// microcode, no vector unit, no RSP memory map.
class RspHle {
 public:
  using CommandHandler = std::function<void(RspHle&, uint64_t command)>;

  RspHle(RdRam& rdram, GfxBackend& gfx);

  // Registers (or overrides) the handler for a display-list opcode. Lets
  // callers add support for a new microcode command without touching this
  // class -- the intended "add an instruction in 5 minutes" extension
  // point.
  void RegisterCommand(uint8_t opcode, CommandHandler handler);

  // Walks a display list in RDRAM starting at dl_address, dispatching each
  // 8-byte command to its registered handler until a G_ENDDL command (or
  // the end of the buffer) is reached.
  void RunDisplayList(uint32_t dl_address);

  RdRam& rdram() { return rdram_; }
  GfxBackend& gfx() { return gfx_; }

 private:
  RdRam& rdram_;
  GfxBackend& gfx_;
  std::unordered_map<uint8_t, CommandHandler> commands_;
};

}  // namespace n64
