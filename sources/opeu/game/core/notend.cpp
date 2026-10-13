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

// The services the end of a battle calls through its event (pointers are DS offsets).
enum EndService : uint16_t {
  SOUND_CUE       = 0,  // sound cue 10
  WAIT            = 1,  // wait(units)
  SHOW_LOCATION   = 2,  // arrival_location(x, y, side)
  ERASE_UNIT      = 3,  // unit_erase(unit)
  PREPARE_UNIT    = 4,  // arrival_prepare(unit)
  FORMAT          = 5,  // request_format(destination, format, arguments...)
  UNIT_MESSAGE    = 6,  // unit_message(unit, text)
  DAILY_SHOW      = 7,  // daily_show(id, side, callback offset, callback segment)
  ELEMENT_SIDE    = 8,  // element_flag_side(element) -> side
  UNIT_NAME       = 9,  // unit_name(unit, buffer)
  ITEM_NAME       = 10, // item_name(item, buffer)
  TYPE_NAME       = 11, // type_name(type, buffer)
  COPY_STRING     = 12, // string_copy(destination, source)
  APPEND_STRING   = 13, // string_append(destination, source)
  REPORT_ELEMENT  = 14, // the panel of an element of a unit
  REPORT_ITEM     = 15, // the panel of a garrison element's equipment
  CONTROL_MESSAGE = 16, // control_message(text, person)
  TURN_PREPARE    = 17, // turn_prepare()
  DAILY_CONTEXT   = 18, // daily_context(side)
};

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
void opeu_end_occupy_present(Game& game, HostEvent event) {
  Call(game, event, SOUND_CUE);
  Call(game, event, WAIT, 30);
}

// The defender retreats in battle_direction; when the battle cell is then empty and the attacker may enter it (an
// objective there must be his own side's), the attacker moves in, and an objective taken is reported and shown.
void opeu_end_occupy(Game& game, HostEvent event) {
  ServiceLocals const locals{ 0x6c };
  auto&               data{ Data(game) };
  uint16_t const      text{ locals.At(0) };
  data.view_side     = 255;
  data.overview_side = 255;
  Call(game, event, SHOW_LOCATION, data.battle_x, data.battle_y, (uint8_t)(data.side_status[0] == SIDE_PLAYED ? 0 : 1));
  uint16_t const objective{ opeu_find_objective_at(game, data.battle_x, data.battle_y) };
  uint16_t       unit;
  if (data.battle_direction != NONE && (unit = opeu_find_unit_at(game, data.battle_x, data.battle_y)) != 0) {
    auto& defender{ Record<Unit>(data, unit) };
    Call(game, event, ERASE_UNIT, unit);
    opeu_grid_step(&defender.x, &defender.y, data.battle_direction);
    Call(game, event, PREPARE_UNIT, unit);
    defender.moved = 0;
  }
  if ((unit = data.battle_units[0]) == 0) return;
  if (opeu_cell_side(game, data.battle_x, data.battle_y) != NO_SIDE) return;
  uint8_t const side{ opeu_unit_side(unit) };
  if (objective && opeu_objective_side(game, objective) != side) return;
  if (!opeu_end_can_occupy(game, data.battle_x, data.battle_y, unit)) return;
  auto& attacker{ Record<Unit>(data, unit) };
  Call(game, event, ERASE_UNIT, unit);
  attacker.x = data.battle_x;
  attacker.y = data.battle_y;
  Call(game, event, PREPARE_UNIT, unit);
  attacker.moved = 0;
  if (objective) {
    Call(game, event, FORMAT, text, offsetof(DataSegment, end_occupied_format), objective);
    Call(game, event, UNIT_MESSAGE, data.current_unit_ptr, text);
    Call(game, event, DAILY_SHOW, OCCUPIED_SHOW, side, OCCUPY_PRESENT_OFFSET, OCCUPY_PRESENT_SEGMENT);
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
void opeu_end_losses(Game& game, HostEvent event) {
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
    auto const side{ static_cast<uint8_t>(Call(game, event, ELEMENT_SIDE, element)) };
    uint16_t   unit;
    if (!loss.unit) {
      unit = GARRISON_AT;
      Call(game, event, COPY_STRING, unit_name, offsetof(DataSegment, end_garrison_name));
      Call(game, event, TYPE_NAME, Near(data, data.element_types[loss.item.type]), item_name);
      Call(game, event, REPORT_ITEM, element + offsetof(Element, type));
    } else {
      unit = opeu_find_element_unit(game, element);
      Call(game, event, UNIT_NAME, loss.unit, unit_name);
      Call(game, event, ITEM_NAME, Near(data, loss.item), item_name);
      Call(game, event, REPORT_ELEMENT, element);
    }
    uint16_t const remaining{ Record<Element>(data, element).quantity };
    uint16_t const lost{ static_cast<uint16_t>(loss.item.quantity - remaining) };
    Call(game, event, FORMAT, report, offsetof(DataSegment, end_loss_format), SIDE_COLOUR[side == 0 ? 0 : 1], data.side_names[side], unit_name, item_name);
    if (!unit || !Record<Element>(data, element).quantity)
      Call(game, event, APPEND_STRING, report, offsetof(DataSegment, end_wiped_out_text));
    else if (!lost)
      Call(game, event, APPEND_STRING, report, offsetof(DataSegment, end_undamaged_text));
    else {
      Call(game, event, FORMAT, loss_text, lost == 1 ? offsetof(DataSegment, end_one_loss_format) : offsetof(DataSegment, end_losses_format), lost);
      Call(game, event, APPEND_STRING, report, loss_text);
    }
    Call(game, event, CONTROL_MESSAGE, report, opeu_find_side_record(game, opeu_side_for_status(game, side), 3));
  }
}

// The end of a battle. Afterwards the daily context of the side a player
// commands is shown again (side_control 1: a player; 0: the computer); with two computer sides, side 1's when a
// player plays it, else side 0's.
void opeu_notice_end(Game& game, HostEvent event) {
  auto& data{ Data(game) };
  Call(game, event, TURN_PREPARE);
  opeu_end_losses(game, event);
  opeu_end_select(game, 0);
  opeu_end_select(game, 1);
  opeu_end_occupy(game, event);
  opeu_end_leaders(game);
  opeu_end_morale(game);
  opeu_end_orders(game);
  opeu_end_nothing();
  auto const show_context{ [&](uint8_t side) {
    data.daily_active = 1;
    Call(game, event, DAILY_CONTEXT, side);
    data.daily_active = 0;
  } };
  if (data.side_control[1] == SIDE_COMPUTER && data.side_control[0] == SIDE_PLAYED) show_context(0);
  if (data.side_control[0] == SIDE_COMPUTER && data.side_control[1] == SIDE_PLAYED) show_context(1);
  if (data.side_control[0] == SIDE_COMPUTER && data.side_control[1] == SIDE_COMPUTER) {
    data.context_side = 255;
    show_context(static_cast<uint8_t>(data.side_status[1] == SIDE_PLAYED));
  }
}

uint16_t svc_notend(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case SOUND_CUE:     opeu_sound_cue10(game, svc_soundcue); break;
  case WAIT:          opeu_delay_units(game, (uint8_t)Arg(args, count, 0)); break;
  case SHOW_LOCATION: opeu_arrival_location(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case ERASE_UNIT:    opeu_unit_remove_draw(game, Arg(args, count, 0)); break;
  case PREPARE_UNIT:  opeu_arrival_prepare(game, Arg(args, count, 0)); break;
  case FORMAT:        opeu_request_format(game, { args, count }); break;
  case UNIT_MESSAGE:  opeu_unit_report(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case DAILY_SHOW:   opeu_daily_show(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint32_t)Arg(args, count, 3) << 16 | Arg(args, count, 2)); break;
  case ELEMENT_SIDE: r = opeu_battle_element_side(game, Arg(args, count, 0)); break;
  case UNIT_NAME:    r = opeu_unit_name(game, svc_unitname, Arg(args, count, 0), Arg(args, count, 1)); break;
  case ITEM_NAME:    r = opeu_item_name(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case TYPE_NAME:    r = opeu_type_name(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case COPY_STRING:  r = opeu_string_copy(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case APPEND_STRING:   r = opeu_string_append(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case REPORT_ELEMENT:  opeu_group_element_panel(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case REPORT_ITEM:     opeu_element_panel(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case CONTROL_MESSAGE: opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case TURN_PREPARE:    opeu_turn_prepare(game, (const DialogHost*)port_dialogs_host()); break;
  case DAILY_CONTEXT:   opeu_daily_context(game, (uint8_t)Arg(args, count, 0)); break;
  default:              port_unhandled("notend", id, count);
  }
  return r;
}

auto port_routine_opeu_end_occupy_present(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_end_occupy_present(game, svc_notend);
  return 0;
}
