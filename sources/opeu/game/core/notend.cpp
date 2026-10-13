// The end of a battle: the winner occupies the emptied cell, morale and leaders' experience change, interrupted
// orders resume, the losses are reported and the daily context is shown again.
#include "core/notend.hpp"
#include "common/events.hpp"
#include "core/counter.hpp"
#include "core/elemfind.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "map/grid.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/select.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/strings.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/btllist.hpp"
#include "map/maperase.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/labels.hpp"
#include "units/unitname.hpp"

#include <array>
#include <cstddef>
#include <span>

using namespace opeu::game;

namespace {

// battle_direction and battle_side when the battle has no retreat and no winner.
constexpr uint8_t NONE{ 255 };

// The saved order of a unit when it has none.
constexpr uint8_t NO_ORDER{ 255 };

// The daily show of an occupied objective and the far address of the routine it calls back
// (opeu_end_occupy_present).
constexpr uint16_t OCCUPIED_SHOW{ 162 };
constexpr uint16_t OCCUPY_PRESENT_OFFSET{ 0x0082 };
constexpr uint16_t OCCUPY_PRESENT_SEGMENT{ 0x3424 };

// The column of move_costs each equipment category moves by (5 columns per terrain); 255 there bars the terrain.
constexpr std::array<uint8_t, 8> OCCUPY_CLASS{ 0, 1, 2, 3, 4, 3, 1, 1 };

// The colour codes the loss report shows each side's name in.
constexpr uint16_t SIDE_COLOUR[]{ '6', '5' };

// The elements of one side.
auto SideElements(DataSegment& data, uint8_t side) -> std::span<Element> {
  return std::span{ data.elements }.subspan(side * ELEMENTS_PER_SIDE, ELEMENTS_PER_SIDE);
}

// Adds a point of experience to a general unless he is a deputy.
void GainExperience(DataSegment& data, uint16_t person) {
  if (!person) return;
  auto& general{ Record<Person>(data, person) };
  if (!(general.role & Bits(PersonRole::DEPUTY))) opeu_add_byte(1, 99, &general.experience);
}

// Resumes the saved order of a unit of a computer-commanded division.
void ResumeOrder(Game& game, uint16_t unit) {
  auto& data{ Data(game) };
  if (unit && (Record<Group>(data, opeu_unit_group(unit)).flags & Bits(GroupFlag::AI_MARK)) && Record<Unit>(data, unit).orders[1].kind != NO_ORDER) opeu_restore_unit_order(game, unit);
}

} // namespace

// Whether every element of the unit may enter the cell (the unit has elements and none is barred by the terrain).
uint16_t opeu_end_can_occupy(Game& game, uint8_t x, uint8_t y, uint16_t unit) {
  auto&      data{ Data(game) };
  auto const first{ Record<Unit>(data, unit).first };
  if (!first) return 0;
  uint8_t const terrain{ opeu_map_code(game, x, y) };
  for (auto const& element : Elements(data, first)) {
    auto const category{ data.element_types[element.type].category };
    if (data.move_costs[terrain * 5 + OCCUPY_CLASS[category]] == IMPASSABLE_COST) return 0;
  }
  return 1;
}

// The call-back of the occupied objective's show: a sound and a pause.
void opeu_end_occupy_present(Game& game) {
  opeu_sound_cue10(game);
  opeu_delay_units(game, static_cast<uint8_t>(30));
}

// The defender retreats in battle_direction; when the battle cell is then empty and the attacker may enter it (an
// objective there must be his own side's), the attacker moves in, and an objective taken is reported and shown.
void opeu_end_occupy(Game& game) {
  ServiceLocals const locals{ 0x6c };
  auto&               data{ Data(game) };
  uint16_t const      text{ locals.At(0) };
  data.view_side     = 255;
  data.overview_side = 255;
  opeu_arrival_location(game, static_cast<uint8_t>(data.battle_x), static_cast<uint8_t>(data.battle_y), (uint8_t)(uint8_t)(data.side_status[0] == SIDE_PLAYED ? 0 : 1));
  uint16_t const objective{ opeu_find_objective_at(game, data.battle_x, data.battle_y) };
  uint16_t       unit;
  if (data.battle_direction != NONE && (unit = opeu_find_unit_at(game, data.battle_x, data.battle_y)) != 0) {
    auto& defender{ Record<Unit>(data, unit) };
    opeu_unit_remove_draw(game, unit);
    opeu_grid_step(&defender.x, &defender.y, data.battle_direction);
    opeu_arrival_prepare(game, unit);
    defender.moved = 0;
  }
  if ((unit = data.battle_units[0]) == 0) return;
  if (opeu_cell_side(game, data.battle_x, data.battle_y) != NO_SIDE) return;
  uint8_t const side{ opeu_unit_side(unit) };
  if (objective && opeu_objective_side(game, objective) != side) return;
  if (!opeu_end_can_occupy(game, data.battle_x, data.battle_y, unit)) return;
  auto& attacker{ Record<Unit>(data, unit) };
  opeu_unit_remove_draw(game, unit);
  attacker.x = data.battle_x;
  attacker.y = data.battle_y;
  opeu_arrival_prepare(game, unit);
  attacker.moved = 0;
  if (objective) {
    opeu_request_format(
        game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(offsetof(DataSegment, end_occupied_format)), static_cast<std::uint16_t>(objective) });
    opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), data.current_unit_ptr, text);
    opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), OCCUPIED_SHOW, static_cast<uint8_t>(side), static_cast<uint32_t>(OCCUPY_PRESENT_SEGMENT) << 16 | OCCUPY_PRESENT_OFFSET);
  }
}

// The winner's (battle_side's) ready elements gain 3..5 morale (capped at 99), the loser's lose 3..5.
void opeu_end_morale(Game& game) {
  auto& data{ Data(game) };
  if (data.battle_side == NONE) return;
  for (auto& element : SideElements(data, data.battle_side))
    if (element.flags & Bits(ElementFlag::READY)) opeu_add_byte(RandomBelow(game, 3) + 3, 99, &element.morale);
  for (auto& element : SideElements(data, opeu_opposite_side(data.battle_side)))
    if (element.flags & Bits(ElementFlag::READY)) opeu_sub_byte(RandomBelow(game, 3) + 3, &element.morale);
}

// The winner's leaders gain a point of experience: the commander of each division that fought, or the side's
// commander when he took command of the battle.
void opeu_end_leaders(Game& game) {
  auto& data{ Data(game) };
  if (data.battle_side == NONE) return;
  // The divisions that fought: those of the defenders' list when no attacker is listed, else of the attackers'.
  std::array<bool, GROUPS_PER_SIDE> fought{};
  uint8_t const                     list{ !data.battle_units[0] };
  for (uint8_t n = 0; n < 9; ++n) {
    uint16_t const unit{ data.battle_units[list * 9 + n] };
    if (unit && unit != GARRISON_AT) fought[opeu_unit_group_index(unit)] = true;
  }
  if (!data.battle_command[data.battle_side]) {
    for (uint8_t group = 0; group < GROUPS_PER_SIDE; ++group)
      if (fought[group]) GainExperience(data, opeu_find_group_record(game, data.battle_side, group));
  } else {
    GainExperience(data, opeu_find_side_record(game, data.battle_side, 1));
  }
}

// After the battle every ready division of the side keeps a selected unit, its commanders' counters of divisions
// gone (not in reserve) restart, and the side keeps a selected division.
void opeu_end_select(Game& game, uint8_t side) {
  auto&                             data{ Data(game) };
  std::array<bool, GROUPS_PER_SIDE> has_selection{};
  bool                              group_selected{ false };
  for (uint8_t g = 0; g < GROUPS_PER_SIDE; g++) {
    auto const& group{ GroupOf(data, side, g) };
    if (group.flags & Bits(GroupFlag::READY)) {
      if (group.flags & Bits(GroupFlag::SELECTED)) group_selected = true;
      for (uint8_t u = 0; u < UNITS_PER_GROUP; u++) {
        auto const& unit{ data.units[(side * GROUPS_PER_SIDE + g) * UNITS_PER_GROUP + u] };
        if ((unit.flags & Bits(UnitFlag::READY)) && (unit.flags & Bits(UnitFlag::SELECTED))) has_selection[g] = true;
      }
    } else {
      if (!(group.flags & Bits(GroupFlag::RESERVE))) {
        if (uint16_t const person{ opeu_find_group_record(game, side, g) }) opeu_set_record_counter(Record<Person>(data, person), 0);
      }
      has_selection[g] = true;
    }
  }
  for (uint8_t g = 0; g < GROUPS_PER_SIDE; g++)
    if (!has_selection[g]) opeu_select_first_unit(game, Near(data, GroupOf(data, side, g)));
  if (!group_selected) opeu_select_first_group(game, side);
}

// The units of computer-commanded divisions on the battle cell and on the cell the defender retreated to resume
// their saved orders.
void opeu_end_orders(Game& game) {
  auto& data{ Data(game) };
  if (data.battle_direction == NONE) return;
  ResumeOrder(game, opeu_find_unit_at(game, data.battle_x, data.battle_y));
  uint8_t x{ data.battle_x };
  uint8_t y{ data.battle_y };
  opeu_grid_step(&x, &y, data.battle_direction);
  ResumeOrder(game, opeu_find_unit_at(game, x, y));
}

void opeu_end_nothing(void) { }

// Reports, when the battle is reported and not only presented, every engaged element's losses to its side's
// commander: wiped out, not damaged, or how many it lost.
void opeu_end_losses(Game& game) {
  ServiceLocals const locals{ 0xb2 };
  auto&               data{ Data(game) };
  uint16_t const      report{ locals.At(0x42) };
  uint16_t const      unit_name{ locals.At(0x36) };
  uint16_t const      loss_text{ locals.At(0x16) };
  uint16_t const      item_name{ locals.At(0) };
  if (data.presentation_mode == 1) return;
  if (!data.battle_report) return;
  for (auto const& loss : data.battle_losses) {
    uint16_t const element{ loss.element };
    if (!element) continue;
    auto const side{ static_cast<uint8_t>(opeu_battle_element_side(game, element)) };
    uint16_t   unit;
    if (!loss.unit) {
      unit = GARRISON_AT;
      opeu_string_copy(game, unit_name, offsetof(DataSegment, end_garrison_name));
      opeu_type_name(game, Near(data, data.element_types[loss.item.type]), item_name);
      opeu_element_panel(game, static_cast<const DialogHost*>(port_dialogs_host()), element + offsetof(Element, type));
    } else {
      unit = opeu_find_element_unit(game, element);
      opeu_unit_name(game, loss.unit, unit_name);
      opeu_item_name(game, Near(data, loss.item), item_name);
      opeu_group_element_panel(game, static_cast<const DialogHost*>(port_dialogs_host()), element);
    }
    uint16_t const remaining{ Record<Element>(data, element).quantity };
    uint16_t const lost{ static_cast<uint16_t>(loss.item.quantity - remaining) };
    opeu_request_format(game, std::array<std::uint16_t, 6>{ static_cast<std::uint16_t>(report), static_cast<std::uint16_t>(offsetof(DataSegment, end_loss_format)),
                                                            static_cast<std::uint16_t>(SIDE_COLOUR[side == 0 ? 0 : 1]), static_cast<std::uint16_t>(data.side_names[side]),
                                                            static_cast<std::uint16_t>(unit_name), static_cast<std::uint16_t>(item_name) });
    if (!unit || !Record<Element>(data, element).quantity)
      opeu_string_append(game, report, offsetof(DataSegment, end_wiped_out_text));
    else if (!lost)
      opeu_string_append(game, report, offsetof(DataSegment, end_undamaged_text));
    else {
      opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(loss_text),
                                                              static_cast<std::uint16_t>(lost == 1 ? offsetof(DataSegment, end_one_loss_format) : offsetof(DataSegment, end_losses_format)),
                                                              static_cast<std::uint16_t>(lost) });
      opeu_string_append(game, report, loss_text);
    }
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), report, opeu_find_side_record(game, opeu_side_for_status(game, side), 3));
  }
}

// The end of a battle. Afterwards the daily context of the side a player
// commands is shown again (side_control 1: a player; 0: the computer); with two computer sides, side 1's when a
// player plays it, else side 0's.
void opeu_notice_end(Game& game) {
  auto& data{ Data(game) };
  opeu_turn_prepare(game, static_cast<const DialogHost*>(port_dialogs_host()));
  opeu_end_losses(game);
  opeu_end_select(game, 0);
  opeu_end_select(game, 1);
  opeu_end_occupy(game);
  opeu_end_leaders(game);
  opeu_end_morale(game);
  opeu_end_orders(game);
  opeu_end_nothing();
  auto const show_context{ [&](uint8_t side) {
    data.daily_active = 1;
    opeu_daily_context(game, static_cast<uint8_t>(side));
    data.daily_active = 0;
  } };
  if (data.side_control[1] == SIDE_COMPUTER && data.side_control[0] == SIDE_PLAYED) show_context(0);
  if (data.side_control[0] == SIDE_COMPUTER && data.side_control[1] == SIDE_PLAYED) show_context(1);
  if (data.side_control[0] == SIDE_COMPUTER && data.side_control[1] == SIDE_COMPUTER) {
    data.context_side = 255;
    show_context(static_cast<uint8_t>(data.side_status[1] == SIDE_PLAYED));
  }
}

auto port_routine_opeu_end_occupy_present(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_end_occupy_present(game);
  return 0;
}
