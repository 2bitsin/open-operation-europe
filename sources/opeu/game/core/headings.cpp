// The army heading (side name, commander, HQ) and the division heading (number, commander, portrait) of the panels.
#include "core/headings.hpp"
#include "common/events.hpp"
#include "core/sideutil.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t ARMY_PERSON{ offsetof(DataSegment, army_person_format) };
constexpr std::uint16_t ARMY_HQ{ offsetof(DataSegment, army_hq_label) };
constexpr std::uint16_t DIVISION_LABEL{ offsetof(DataSegment, division_label_format) };
constexpr std::uint16_t DIVISION_PERSON{ offsetof(DataSegment, division_person_format) };
constexpr std::uint16_t COLOR_WHITE{ 7 };

} // namespace

void opeu_army_heading(Game& game, uint16_t person) {
  auto const& data{ Data(game) };
  opeu_ui_fill(6, 0, 73, 50, 0);
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 6, 0, 73, 51, COLOR_WHITE);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(1), static_cast<int16_t>(2));
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), data.side_names[data.current_side]);
  opeu_select_side_color(game, data.current_side);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(1), static_cast<int16_t>(18));
  opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(ARMY_PERSON), static_cast<std::uint16_t>(SideTextColor(data.current_side)), static_cast<std::uint16_t>(person) });
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(1), static_cast<int16_t>(34));
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(ARMY_HQ) });
  opeu_ui_line(40, 51, 40, 186, COLOR_WHITE);
}

void opeu_division_heading(Game& game, uint16_t person) {
  auto& data{ Data(game) };
  opeu_ui_fill(6, 143, 73, 256, 0);
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 5, 142, 73, 257, COLOR_WHITE);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(1), static_cast<int16_t>(225));
  opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(DIVISION_LABEL), static_cast<std::uint16_t>(SideTextColor(data.current_side)),
                                                        static_cast<std::uint16_t>((Record<Person>(data, person).assignment & Bits(Assignment::DIVISION_MASK)) + 1) });
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(1), static_cast<int16_t>(241));
  opeu_cursor_print(game,
                    std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(DIVISION_PERSON), static_cast<std::uint16_t>(SideTextColor(data.current_side)), static_cast<std::uint16_t>(person) });
  opeu_context_portrait(game, static_cast<const UiHost*>(port_uiprim_host()), 1, 144, person);
}
