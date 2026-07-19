#include "n64/controller.h"
#include "n64/rdram.h"
#include "n64/sprite_scene.h"
#include "test_framework.h"

TEST_MAIN_BEGIN()
n64::SpriteScene scene;
const n64::SpriteScene::Position start = scene.position();

// No input: the square doesn't move.
{
  n64::Controller pad;
  scene.Advance(pad);
  CHECK(scene.position().x == start.x);
  CHECK(scene.position().y == start.y);
}

// D-Pad right moves it right by kSpeed; D-Pad down moves it down by kSpeed.
{
  n64::Controller pad;
  pad.SetButton(n64::Controller::Button::DRight, true);
  scene.Advance(pad);
  CHECK(scene.position().x == start.x + n64::SpriteScene::kSpeed);
  CHECK(scene.position().y == start.y);
}
{
  n64::Controller pad;
  pad.SetButton(n64::Controller::Button::DDown, true);
  scene.Advance(pad);
  CHECK(scene.position().y == start.y + n64::SpriteScene::kSpeed);
}

// The stick works too, past the deadzone threshold.
{
  n64::SpriteScene stick_scene;
  const n64::SpriteScene::Position stick_start = stick_scene.position();
  n64::Controller pad;
  pad.SetStick(100, 0);
  stick_scene.Advance(pad);
  CHECK(stick_scene.position().x == stick_start.x + n64::SpriteScene::kSpeed);
}

// Movement clamps at the screen edges rather than going out of bounds.
{
  n64::SpriteScene edge_scene;
  n64::Controller pad;
  pad.SetButton(n64::Controller::Button::DLeft, true);
  for (int i = 0; i < 1000; ++i) {
    edge_scene.Advance(pad);
  }
  CHECK(edge_scene.position().x == 0);
}
{
  n64::SpriteScene edge_scene;
  n64::Controller pad;
  pad.SetButton(n64::Controller::Button::DRight, true);
  for (int i = 0; i < 1000; ++i) {
    edge_scene.Advance(pad);
  }
  CHECK(edge_scene.position().x ==
        static_cast<int>(n64::SpriteScene::kWidth - n64::SpriteScene::kSquareSize));
}

// Draw() writes the square color inside the square's bounds and the
// background color outside it.
{
  n64::SpriteScene draw_scene;
  n64::RdRam rdram;
  constexpr uint32_t kOrigin = 0x1000;
  draw_scene.Draw(rdram, kOrigin);

  const n64::SpriteScene::Position pos = draw_scene.position();
  const uint32_t inside_offset =
      kOrigin +
      ((static_cast<uint32_t>(pos.y) * n64::SpriteScene::kWidth) + static_cast<uint32_t>(pos.x)) *
          2;
  CHECK(rdram.Read16(inside_offset) == 0xFFFF);

  const uint32_t outside_offset = kOrigin;  // Top-left corner, outside the centered square.
  CHECK(rdram.Read16(outside_offset) == 0x0001);
}
TEST_MAIN_END()
