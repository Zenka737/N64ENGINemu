#include <cstdint>

#include "n64/controller.h"
#include "test_framework.h"

using n64::Controller;
using Button = n64::Controller::Button;

TEST_MAIN_BEGIN()
Controller pad;

// Fresh controller reports nothing pressed and a centered stick.
CHECK(pad.buttons() == 0);
CHECK(pad.stick_x() == 0);
CHECK(pad.stick_y() == 0);
CHECK(pad.Packed() == 0);

// Each button maps to its documented hardware bit position.
CHECK(static_cast<uint16_t>(Button::A) == 0x8000);
CHECK(static_cast<uint16_t>(Button::B) == 0x4000);
CHECK(static_cast<uint16_t>(Button::Z) == 0x2000);
CHECK(static_cast<uint16_t>(Button::Start) == 0x1000);
CHECK(static_cast<uint16_t>(Button::DUp) == 0x0800);
CHECK(static_cast<uint16_t>(Button::DDown) == 0x0400);
CHECK(static_cast<uint16_t>(Button::DLeft) == 0x0200);
CHECK(static_cast<uint16_t>(Button::DRight) == 0x0100);
CHECK(static_cast<uint16_t>(Button::Reset) == 0x0080);
CHECK(static_cast<uint16_t>(Button::L) == 0x0020);
CHECK(static_cast<uint16_t>(Button::R) == 0x0010);
CHECK(static_cast<uint16_t>(Button::CUp) == 0x0008);
CHECK(static_cast<uint16_t>(Button::CDown) == 0x0004);
CHECK(static_cast<uint16_t>(Button::CLeft) == 0x0002);
CHECK(static_cast<uint16_t>(Button::CRight) == 0x0001);

// SetButton / IsPressed round-trip and only affect the targeted bit.
pad.SetButton(Button::A, true);
CHECK(pad.IsPressed(Button::A));
CHECK(!pad.IsPressed(Button::B));
CHECK(pad.buttons() == 0x8000);

pad.SetButton(Button::Start, true);
CHECK(pad.buttons() == 0x9000);
CHECK(pad.IsPressed(Button::Start));

pad.SetButton(Button::A, false);
CHECK(!pad.IsPressed(Button::A));
CHECK(pad.buttons() == 0x1000);

// Packed layout: buttons in the high 16 bits.
CHECK(pad.Packed() == 0x1000'0000U);

// Signed stick bytes survive the round-trip and pack into the low bytes.
pad.SetStick(-1, 127);
CHECK(pad.stick_x() == -1);
CHECK(pad.stick_y() == 127);
CHECK(pad.Packed() == 0x1000'FF7FU);

pad.SetStick(-128, -128);
CHECK(pad.Packed() == 0x1000'8080U);

pad.SetStick(80, -80);
CHECK(pad.Packed() == 0x1000'50B0U);

// A fully documented example word: A + D-Right held, stick pushed up-right.
Controller combo;
combo.SetButton(Button::A, true);
combo.SetButton(Button::DRight, true);
combo.SetStick(80, 80);
CHECK(combo.buttons() == 0x8100);
CHECK(combo.Packed() == 0x8100'5050U);

// Reset clears everything.
combo.Reset();
CHECK(combo.buttons() == 0);
CHECK(combo.stick_x() == 0);
CHECK(combo.stick_y() == 0);
CHECK(combo.Packed() == 0);
TEST_MAIN_END()
