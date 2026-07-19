#include <vector>

#include "n64/frame_timing.h"
#include "test_framework.h"

TEST_MAIN_BEGIN()
// Instruction budget is clock / fps, with sane edge handling.
CHECK(n64::InstructionsPerFrame(60'000'000ULL, 60) == 1'000'000ULL);
CHECK(n64::InstructionsPerFrame(n64::kVr4300ClockHz, 60) == n64::kVr4300ClockHz / 60);
CHECK(n64::InstructionsPerFrame(93'750'000ULL, 0) == 0);
CHECK(n64::InstructionsPerFrame(93'750'000ULL, -1) == 0);

// Test pattern fills the whole RGBA buffer and keeps alpha opaque.
std::vector<uint8_t> pixels;
n64::FillTestPattern(pixels, {4, 3}, 0);
CHECK(pixels.size() == static_cast<size_t>(4 * 3 * 4));
CHECK(pixels[3] == 0xFF);
CHECK(pixels[pixels.size() - 1] == 0xFF);

// Advancing the frame phase changes pixel values (animation is visible).
std::vector<uint8_t> next;
n64::FillTestPattern(next, {4, 3}, 1);
CHECK(next[0] != pixels[0]);

// Degenerate dimensions produce an empty buffer instead of crashing.
n64::FillTestPattern(pixels, {0, 3}, 0);
CHECK(pixels.empty());
TEST_MAIN_END()
