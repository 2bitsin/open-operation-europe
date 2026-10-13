// The conversion team's credits, shown after the division panel when the player answers its key prompt with '!'.
#include "core/credits.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/mouse.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t CREDITS_TITLE{ offsetof(DataSegment, credits_title) };
constexpr std::uint16_t CREDITS_MAIN{ offsetof(DataSegment, credits_main) };
constexpr std::uint16_t CREDITS_SUPPORT{ offsetof(DataSegment, credits_support) };
constexpr std::uint16_t CREDITS_TRANSLATION{ offsetof(DataSegment, credits_translation) };
constexpr std::uint16_t CREDITS_X{ 17 };
} // namespace

// Waits for the panel's key; on '!' clears the screen to color 1, flags a restart of the display and shows the
// credits until a key or button is pressed. With the mouse input mode on, the mouse answers the prompt instead.
void opeu_division_credits(Game& game) {
  auto& data{ Data(game) };
  opeu_input_flush(game, static_cast<TextHost*>(port_textout_host()));
  while (data.input_mode)
    if (static_cast<uint16_t>(opeu_turn_input_a(game))) return;
  if (static_cast<char>(static_cast<uint16_t>(port_getch())) == '!') {
    opeu_backdrop_fill_color(game, static_cast<const UiHost*>(port_uiprim_host()), 1);
    data.restart_flag = 1;
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), CREDITS_X, 100);
    opeu_cursor_print(game, std::array<std::uint16_t, 1>{ CREDITS_TITLE });
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), CREDITS_X, 150);
    opeu_cursor_print(game, std::array<std::uint16_t, 1>{ CREDITS_MAIN });
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), CREDITS_X, 170);
    opeu_cursor_print(game, std::array<std::uint16_t, 1>{ CREDITS_SUPPORT });
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), CREDITS_X, 190);
    opeu_cursor_print(game, std::array<std::uint16_t, 1>{ CREDITS_TRANSLATION });
    while (!opeu_turn_input_b()) {
    }
  }
}
