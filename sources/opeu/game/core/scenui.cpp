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
#include "link/glue.hpp"
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
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x1e, 0x10, 0x113, 0x72, 2);
  data.scenario_text_x = 4;
  data.scenario_text_y = 0x18;
}

void opeu_scenario_message(Game& game, uint16_t text) {
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(4), static_cast<int16_t>(0x19));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(MESSAGE_FORMAT), static_cast<std::uint16_t>(text) });
}

void opeu_scenario_clear_panel(Game& game) {
  opeu_ui_box(3, 8, 0x22, 0x170, 7);
}

void opeu_scenario_clear_text(Game& game) {
  opeu_ui_box(2, 0x86, 0x22, 0xb8, 7);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(4), static_cast<int16_t>(0xe0));
}

// The scenario description, scrolled inside its window.
void opeu_scenario_description(Game& game, uint16_t text) {
  opeu_text_scroll_mode(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(1));
  opeu_text_window(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(4), static_cast<int16_t>(0x25), static_cast<int16_t>(0x1a), static_cast<int16_t>(0x164));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(4), static_cast<int16_t>(0x1e));
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), text);
  opeu_text_window_reset(game, static_cast<TextHost*>(port_textout_host()));
  opeu_text_scroll_mode(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0));
}
