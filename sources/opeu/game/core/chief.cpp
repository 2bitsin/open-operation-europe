// The operations chief's portrait panel, and the messages, waits and number prompts shown beside it.
#include "core/chief.hpp"
#include "common/events.hpp"
#include "core/personfl.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/keyboard.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t CHIEF_FORMAT{ offsetof(DataSegment, ops_chief_format) };
constexpr std::uint16_t RANGE_FORMAT{ offsetof(DataSegment, number_range_format) };
constexpr std::uint16_t RANGE_TEXT_BYTES{ 0x1e }; // the "(low-high)" text
constexpr std::uint16_t PANEL_X{ 0x35 };
constexpr std::uint16_t PANEL_Y{ 0x130 };

auto Chief(Game& game) -> std::uint16_t {
  return Data(game).context_record2;
}
} // namespace

// Draws the panel: a frame wide enough for the chief's name, the name and the portrait; leaves the text cursor in it.
uint16_t opeu_ops_chief_show(Game& game) {
  opeu_person_lacks_side0_flag(game, Chief(game));
  game.Machine().Display().Box({ PANEL_X, PANEL_Y, 0x18, 0x40 }, 0);
  opeu_context_frame(game, 0x2a, 0x10a, static_cast<std::uint8_t>(opeu_string_length(game, Chief(game)) + 13));
  opeu_cursor_print(game, std::array<FormatArgument, 2>{ CHIEF_FORMAT, Chief(game) });
  opeu_context_portrait(game, 0x2b, 0x131, Chief(game));
  opeu_army_position(game, PANEL_X, PANEL_Y);
  return 1;
}

void opeu_ops_chief_text(Game& game, Text text) {
  if (opeu_ops_chief_show(game)) opeu_text_feed(game, text);
}

// Shows the text and the range "(low-high)" formatted into a buffer of its own, then reads a number in the range.
uint16_t opeu_number_prompt(Game& game, Text text, uint16_t low, uint16_t high, uint16_t value, uint16_t d) {
  TextBuffer<RANGE_TEXT_BYTES> buffer;
  opeu_request_format(game, buffer, std::array<FormatArgument, 3>{ RANGE_FORMAT, low, high });
  opeu_text_feed(game, text);
  opeu_text_feed(game, buffer);
  return opeu_choose_value(game, { low, high }, { .cancel = static_cast<int16_t>(value), .items = d });
}

// The operations chief asks for a number in low..high.
uint16_t opeu_ops_chief_number(Game& game, Text text, uint16_t low, uint16_t high, uint16_t value, uint16_t d) {
  opeu_ops_chief_show(game);
  return opeu_number_prompt(game, text, low, high, value, d);
}

uint16_t opeu_ops_chief_wait(Game& game, Text text) {
  opeu_ops_chief_show(game);
  return opeu_confirm_prompt(game, text);
}
