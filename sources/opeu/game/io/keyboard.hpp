// Keyboard waits, the text cursor, decimal entry, value choice and yes/no prompts.
#pragma once

#include "common/events.hpp"
#include "io/numdlg.hpp"

#include <cstdint>

namespace opeu::game::detail::keyboard {

// How a value is chosen: whether the player may cancel, and the near address of the mouse menu of the values (0: the
// numeric dialog or decimal entry).
struct ChoiceOptions {
  int           cancel;
  std::uint16_t items;
};

// A choice of a value in a range.
struct ValueChoice {
  ValueRange    range;
  ChoiceOptions options;
};
} // namespace opeu::game::detail::keyboard

namespace opeu::game {
using detail::keyboard::ChoiceOptions;
using detail::keyboard::ValueChoice;
} // namespace opeu::game

void opeu_text_cursor_draw(Game& game);
auto opeu_key_wait_tick(Game& game) -> int;
auto opeu_keyboard_key(Game& game) -> std::uint8_t;
auto opeu_mouse_wait_buttons(Game& game) -> std::uint8_t;
void opeu_wait_confirm(Game& game);
auto opeu_read_decimal(Game& game, std::uint16_t digits) -> std::int32_t;
auto opeu_choose_value(Game& game, opeu::game::ValueRange range, opeu::game::ChoiceOptions options) -> std::uint16_t;
auto opeu_prompt_value(Game& game, std::uint16_t text, opeu::game::ValueChoice const& choice) -> std::uint16_t;
auto opeu_prompt_value_cancel(Game& game, std::uint16_t text, std::uint16_t max, std::uint16_t items) -> std::uint16_t;
auto opeu_prompt_value_required(Game& game, std::uint16_t text, std::uint16_t max, std::uint16_t items) -> std::uint16_t;
auto opeu_mouse_yes_no(Game& game) -> std::uint8_t;
auto opeu_yes_no(Game& game) -> int;
auto opeu_prompt_yes_no(Game& game, std::uint16_t text) -> int;
