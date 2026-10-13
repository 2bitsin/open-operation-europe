// The panels of the scenario-selection screen: the title frame, framed messages, panel clears and the scrolling
// scenario description.
#include "core/scenui.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t MESSAGE_FORMAT{ offsetof(DataSegment, scenario_message_format) };

} // namespace

// The title frame; the scenario text starts below it.
void opeu_scenario_frame(Game& game) {
  auto& data{ Data(game) };
  opeu_ui_outline(game, 0x1e, 0x10, 0x113, 0x72, 2);
  data.scenario_text_x = 4;
  data.scenario_text_y = 0x18;
}

void opeu_scenario_message(Game& game, uint16_t text) {
  opeu_army_position(game, 4, 0x19);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ MESSAGE_FORMAT, text });
}

void opeu_scenario_clear_panel(Game& game) {
  game.Machine().Display().Box({ 3, 8, 0x22, 0x170 }, 7);
}

void opeu_scenario_clear_text(Game& game) {
  game.Machine().Display().Box({ 2, 0x86, 0x22, 0xb8 }, 7);
  opeu_army_position(game, 4, 0xe0);
}

// The scenario description, scrolled inside its window.
void opeu_scenario_description(Game& game, uint16_t text) {
  opeu_text_scroll_mode(game, 1);
  opeu_text_window(game, 4, 0x25, 0x1a, 0x164);
  opeu_army_position(game, 4, 0x1e);
  opeu_ui_text(game, text);
  opeu_text_window_reset(game);
  opeu_text_scroll_mode(game, 0);
}
