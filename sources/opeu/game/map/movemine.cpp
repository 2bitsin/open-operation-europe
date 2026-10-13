// A unit that moves into a minefield: an engineer unit only reports the mines; any other reports the minefield, shows
// it to a player's side that may see the unit and takes casualties. Either way the cell is marked for the side and the
// unit moves on. Also: element damage capped at the quantity, and the current unit's movement rating.
#include "map/movemine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/sideutil.hpp"
#include "map/cellflag.hpp"
#include "map/maperase.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/casualty.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The minefield picture.
constexpr uint16_t MINES_PICTURE{ 0x48 };
} // namespace

void opeu_move_mines(Game& game, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const current{ data.current_unit_ptr };
  if (opeu_category_6(static_cast<uint8_t>(data.current_entry))) {
    opeu_unit_report(game, static_cast<std::uint16_t>(current), static_cast<std::uint16_t>(Near(data, data.mines_found_text)));
  } else {
    opeu_unit_report(game, static_cast<std::uint16_t>(current), static_cast<std::uint16_t>(Near(data, data.minefield_text)));
    auto const own{ data.current_side };
    auto const enemy{ opeu_opposite_side(data.current_side) };
    if (data.side_status[own] == SIDE_PLAYED && opeu_unit_selection_eligible(game, static_cast<std::uint16_t>(current)))
      opeu_daily_show(game, MINES_PICTURE, own, Presenter::MINES_HIT);
    else if (data.side_status[enemy] == SIDE_PLAYED && opeu_unit_special(game, static_cast<std::uint16_t>(current)))
      opeu_daily_show(game, MINES_PICTURE, enemy, Presenter::MINES_HIT);
    opeu_unit_casualties(game, static_cast<std::uint16_t>(current), 100, 3, 15, 0, 1);
  }
  auto& unit{ Record<Unit>(data, current) };
  opeu_mark_cell(game, static_cast<uint8_t>(unit.x), static_cast<uint8_t>(unit.y), static_cast<uint8_t>(data.current_side));
  opeu_unit_remove_draw(game, static_cast<std::uint16_t>(current));
  unit.x     = x;
  unit.y     = y;
  unit.moved = 0;
}

// Caps each element's damage at its quantity.
void opeu_move_clamp(Game& game) {
  auto& data{ Data(game) };
  for (auto& element : Elements(data, Record<Unit>(data, data.current_unit_ptr))) element.damage = opeu_min16(element.quantity, element.damage);
}

// sqrt(max(0, (leader + morale + training) / 3 - experience)), plus the delivery level for a computer side.
uint8_t opeu_move_rating(Game& game) {
  auto const& data{ Data(game) };
  auto const  current{ data.current_unit_ptr };
  auto const  morale{ opeu_unit_average9(game, static_cast<std::uint16_t>(current)) };
  auto const  training{ opeu_unit_average10(game, static_cast<std::uint16_t>(current)) };
  auto const  experience{ opeu_unit_average11(game, static_cast<std::uint16_t>(current)) };
  auto const  leadership{ Record<Person>(data, data.context_group_record).leadership };
  auto        rating{ static_cast<uint16_t>(static_cast<uint16_t>(leadership + morale + training) / 3) };
  auto const  bonus{ data.side_control[data.current_side] == SIDE_COMPUTER ? data.delivery_level : 0 };
  return static_cast<uint8_t>(opeu_isqrt(static_cast<std::uint16_t>(opeu_sub_word(experience, &rating))) + bonus);
}
