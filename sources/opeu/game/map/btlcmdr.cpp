// The battle screen: restoring the panel backgrounds, the commander's name and portrait panel with its messages, and
// the destruction of an element.
#include "map/btlcmdr.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "map/btlcalc.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/btlunit.hpp"
#include "map/hqloss.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>
#include <ranges>

using namespace opeu::game;

namespace {
constexpr uint8_t                LAST_COLUMN{ 39 };
constexpr uint16_t               NAME_BYTES{ 0x0c }; // the panel's DS reserve for the 10-byte name buffer the services write
constexpr std::array<uint8_t, 2> NAME_COLOR{ 6, 5 };
constexpr uint8_t                TEXT_WHITE{ 7 };

// Rows first.. of the screen, each restored from column 0 across widths[row] tiles.
template <typename Widths>
void RestoreLeft(Game& game, Widths const& widths, uint8_t rows, uint8_t first) {
  for (auto const row : std::views::iota(uint8_t{ 0 }, rows))
    for (auto const column : std::views::iota(uint8_t{ 0 }, widths[row])) opeu_battle_restore_tile(game, column, static_cast<uint8_t>(row + first));
}

// The same from the last column leftward.
template <typename Widths>
void RestoreRight(Game& game, Widths const& widths, uint8_t rows, uint8_t first) {
  for (auto const row : std::views::iota(uint8_t{ 0 }, rows))
    for (uint8_t column{ LAST_COLUMN }; LAST_COLUMN - widths[row] < column; --column) opeu_battle_restore_tile(game, column, static_cast<uint8_t>(row + first));
}

// Takes percent of the word away.
void Reduce(uint16_t& value, uint16_t percent) {
  value = SubWord(value, opeu_mul_div(value, percent, 100));
}
} // namespace

void opeu_battle_restore_tile(Game& game, uint8_t column, uint8_t row) {
  game.Machine().Display().CopyColumns({ 0x1e, 0, 2, 0x10 }, { static_cast<uint16_t>(column * 2), static_cast<uint16_t>(row << 4) });
}

void opeu_battle_info_clear(Game& game) {
  RestoreLeft(game, Data(game).battle_info_widths, 4, 5);
}

void opeu_battle_panel_begin(Game& game) {
  RestoreRight(game, Data(game).battle_top_widths, 11, 0);
}

void opeu_battle_panel_side1(Game& game) {
  RestoreLeft(game, Data(game).battle_mid_left_widths, 6, 15);
  opeu_battle_name_clear_left(game);
  Data(game).battle_panel_shown[0] = 0;
}

void opeu_battle_panel_side0(Game& game) {
  RestoreRight(game, Data(game).battle_mid_right_widths, 6, 15);
  opeu_battle_name_clear_right(game);
  Data(game).battle_panel_shown[1] = 0;
}

void opeu_battle_name_clear_left(Game& game) {
  auto& data{ Data(game) };
  RestoreLeft(game, data.battle_low_left_widths, 4, 21);
  opeu_army_position(game, static_cast<short>(1), static_cast<short>(0x158));
  opeu_ui_text(game, Near(data, data.battle_left_mark));
}

void opeu_battle_name_clear_right(Game& game) {
  auto& data{ Data(game) };
  RestoreRight(game, data.battle_low_right_widths, 4, 21);
  opeu_army_position(game, static_cast<short>(0x38), static_cast<short>(0x158));
  opeu_ui_text(game, Near(data, data.battle_right_mark));
}

void opeu_battle_commander_panel(Game& game, uint8_t side, uint16_t refresh) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ NAME_BYTES };
  auto const          name{ locals.At(0) };
  if (refresh) {
    if (side == 0)
      opeu_battle_name_clear_right(game);
    else
      opeu_battle_name_clear_left(game);
  }
  if (data.battle_panel_shown[side]) {
    if (side == 0)
      opeu_battle_name_clear_left(game);
    else
      opeu_battle_name_clear_right(game);
    return;
  }
  if (side == 0)
    opeu_battle_panel_side1(game);
  else
    opeu_battle_panel_side0(game);
  // The defender's first unit, or the attacker's: the garrison is led by the side's second staff post.
  auto const defender{ BattleList(data, 1, 0) };
  auto const ours{ static_cast<uint8_t>(opeu_battle_unit_side(game, defender)) == side };
  uint16_t   person;
  if (ours && defender == Near(data, data.battle_garrison)) {
    person = opeu_find_side_record(game, static_cast<uint8_t>(side), static_cast<uint8_t>(2));
    opeu_string_copy(game, name, data.role_names[1]);
  } else {
    auto const unit{ ours ? defender : BattleList(data, 0, 0) };
    auto const group{ static_cast<uint8_t>(opeu_unit_group_index(unit)) };
    person = opeu_find_group_record(game, static_cast<uint8_t>(side), static_cast<uint8_t>(group));
    opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(name), static_cast<std::uint16_t>(data.battle_division_format), static_cast<std::uint16_t>(group + 1) });
  }
  auto const x{ side ? 0x3c : 9 };
  opeu_context_save(game);
  opeu_context_portrait(game, side ? 0x48 : 0, 0x100, person);
  opeu_army_position(game, static_cast<short>(x), static_cast<short>(0x130));
  opeu_text_color(game, NAME_COLOR[side ? 1 : 0]);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.battle_commander_format)), static_cast<std::uint16_t>(name) });
  opeu_army_position(game, static_cast<short>(x), static_cast<short>(0x140));
  opeu_text_color(game, TEXT_WHITE);
  opeu_ui_text(game, person);
  opeu_context_restore(game);
  data.battle_panel_shown[side] = 1;
}

// Shows the side's commander panel with the wait text and its value.
void opeu_battle_commander_wait(Game& game, uint16_t text, uint8_t side, uint8_t value) {
  opeu_battle_commander_panel(game, side, 1);
  opeu_cursor_print(
      game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(Near(Data(game), Data(game).battle_wait_format)), static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(value + '0') });
}

void opeu_battle_commander_message(Game& game, uint16_t text, uint8_t side, uint8_t value) {
  opeu_battle_commander_panel(game, side, 0);
  opeu_cursor_print(
      game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(Near(Data(game), Data(game).battle_prompt_format)), static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(value + '0') });
}

void opeu_battle_element_destroy(Game& game, uint16_t element) {
  auto& data{ Data(game) };
  opeu_battle_clear_cell(game, static_cast<uint8_t>(PositionOf(data, element).x), static_cast<uint8_t>(PositionOf(data, element).y));
  auto const unit{ opeu_battle_element_unit(game, element) };
  bool const selected{ (Record<Element>(data, element).flags & Bits(ElementFlag::SELECTED)) != 0 };
  opeu_battle_remove_unit(game, unit, element);
  auto& record{ Record<Unit>(data, unit) };
  if (!(record.flags & Bits(UnitFlag::READY))) {
    record.resources[1] = 0;
    record.resources[0] = 0;
    record.resources[2] = 0;
    opeu_battle_remove_list(game, unit);
    return;
  }
  if (selected) {
    data.battle_lost_element = element;
    data.battle_lost_unit    = unit;
    opeu_battle_hq_lost(game);
    data.battle_lost_element = 0;
    data.battle_lost_unit    = 0;
  }
  if (opeu_battle_unit_active(game, unit)) return;
  auto const garrison{ Near(data, data.battle_garrison) };
  if (static_cast<uint8_t>(opeu_battle_unit_direction(game, unit)) == BATTLE_NOT_ADJACENT && unit != garrison) data.battle_direction = data.battle_exit_direction[0];
  if (unit == garrison) {
    auto&   objective{ Record<Objective>(data, opeu_find_objective_at(game, static_cast<uint8_t>(data.battle_x), static_cast<uint8_t>(data.battle_y))) };
    uint8_t gone{ 0 };
    for (auto& other : Elements(data, record))
      if ((other.flags & Bits(ElementFlag::READY)) && (PositionOf(data, Near(data, other)).x & BATTLE_LEFT)) ++gone;
    auto const percent{ game.State().RandomBounded(gone * 10 + 41) };
    Reduce(objective.resources[1], percent);
    Reduce(objective.resources[2], percent);
    Reduce(objective.resources[0], percent);
    for (auto& item : objective.items)
      if (item.quantity) item.quantity = 0;
  }
  opeu_battle_remove_list(game, unit);
}
