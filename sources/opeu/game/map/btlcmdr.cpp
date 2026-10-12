// The battle screen: restoring the panel backgrounds, the commander's name and portrait panel with its messages, and
// the destruction of an element.
#include "types.hpp"
#include "map/btlcmdr.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t  LAST_COLUMN{ 39 };
constexpr uint16_t NAME_AT{
  0x0c
}; // below the panel's frame: the 10-byte name buffer the services write
constexpr uint16_t CALLER_FRAME{
  10
}; // the panel's frame below its callers' frames
constexpr uint8_t NAME_COLOR[2]{ 6, 5 };
constexpr uint8_t TEXT_WHITE{ 7 };

// Rows first.. of the screen, each restored from column 0 across widths[row] tiles.
template <typename Widths>
void RestoreLeft(uint8_t* ds, BattleEvent event, Widths const& widths,
                 uint8_t rows, uint8_t first) {
  for (uint8_t row{ 0 }; row < rows; ++row)
    for (uint8_t column{ 0 }; column < widths[row]; ++column)
      opeu_battle_restore_tile(ds, event, column,
                               static_cast<uint8_t>(row + first));
}

// The same from the last column leftward.
template <typename Widths>
void RestoreRight(uint8_t* ds, BattleEvent event, Widths const& widths,
                  uint8_t rows, uint8_t first) {
  for (uint8_t row{ 0 }; row < rows; ++row)
    for (uint8_t column{ LAST_COLUMN }; LAST_COLUMN - widths[row] < column;
         --column)
      opeu_battle_restore_tile(ds, event, column,
                               static_cast<uint8_t>(row + first));
}

// Takes percent of the word away through the original's services.
void Reduce(uint8_t* ds, BattleEvent event, uint16_t& value, uint16_t percent) {
  auto const amount{ Call(ds, event, BattleRoutine::MUL_DIV, value, percent,
                          100) };
  Call(ds, event, BattleRoutine::SUB_WORD, amount, Near(Data(ds), value));
}
} // namespace

void opeu_battle_restore_tile(uint8_t* ds, BattleEvent event, uint8_t column,
                              uint8_t row) {
  Call(ds, event, BattleRoutine::SCREEN_COPY, 0x1e, 0, 2, 0x10, column * 2,
       row << 4);
}

void opeu_battle_info_clear(uint8_t* ds, BattleEvent event) {
  RestoreLeft(ds, event, Data(ds).battle_info_widths, 4, 5);
}

void opeu_battle_panel_begin(uint8_t* ds, BattleEvent event) {
  RestoreRight(ds, event, Data(ds).battle_top_widths, 11, 0);
}

void opeu_battle_panel_side1(uint8_t* ds, BattleEvent event) {
  RestoreLeft(ds, event, Data(ds).battle_mid_left_widths, 6, 15);
  opeu_battle_name_clear_left(ds, event);
  Data(ds).battle_panel_shown[0] = 0;
}

void opeu_battle_panel_side0(uint8_t* ds, BattleEvent event) {
  RestoreRight(ds, event, Data(ds).battle_mid_right_widths, 6, 15);
  opeu_battle_name_clear_right(ds, event);
  Data(ds).battle_panel_shown[1] = 0;
}

void opeu_battle_name_clear_left(uint8_t* ds, BattleEvent event) {
  auto& data{ Data(ds) };
  RestoreLeft(ds, event, data.battle_low_left_widths, 4, 21);
  Call(ds, event, BattleRoutine::ARMY_POSITION, 1, 0x158);
  Call(ds, event, BattleRoutine::UI_TEXT, Near(data, data.battle_left_mark));
}

void opeu_battle_name_clear_right(uint8_t* ds, BattleEvent event) {
  auto& data{ Data(ds) };
  RestoreRight(ds, event, data.battle_low_right_widths, 4, 21);
  Call(ds, event, BattleRoutine::ARMY_POSITION, 0x38, 0x158);
  Call(ds, event, BattleRoutine::UI_TEXT, Near(data, data.battle_right_mark));
}

// frame: the routine's frame on the DS stack; the name buffer lives there.
void opeu_battle_commander_panel(uint8_t* ds, BattleEvent event, uint8_t side,
                                 uint16_t refresh, uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const name{ Local(frame, NAME_AT) };
  if (refresh) {
    if (side == 0)
      opeu_battle_name_clear_right(ds, event);
    else
      opeu_battle_name_clear_left(ds, event);
  }
  if (data.battle_panel_shown[side]) {
    if (side == 0)
      opeu_battle_name_clear_left(ds, event);
    else
      opeu_battle_name_clear_right(ds, event);
    return;
  }
  if (side == 0)
    opeu_battle_panel_side1(ds, event);
  else
    opeu_battle_panel_side0(ds, event);
  // The defender's first unit, or the attacker's: the garrison is led by the side's second staff post.
  auto const defender{ BattleList(data, 1, 0) };
  auto const ours{ static_cast<uint8_t>(Call(ds, event,
                                             BattleRoutine::BATTLE_UNIT_SIDE,
                                             defender)) == side };
  uint16_t   person;
  if (ours && defender == Near(data, data.battle_garrison)) {
    person = Call(ds, event, BattleRoutine::FIND_SIDE_RECORD, side, 2);
    Call(ds, event, BattleRoutine::STRING_COPY, name, data.role_names[1]);
  } else {
    auto const unit{ ours ? defender : BattleList(data, 0, 0) };
    auto const group{ static_cast<uint8_t>(
        Call(ds, event, BattleRoutine::UNIT_GROUP_INDEX, unit)) };
    person = Call(ds, event, BattleRoutine::FIND_GROUP_RECORD, side, group);
    Call(ds, event, BattleRoutine::REQUEST_FORMAT, name,
         data.battle_division_format, group + 1);
  }
  auto const x{ side ? 0x3c : 9 };
  Call(ds, event, BattleRoutine::CONTEXT_SAVE);
  Call(ds, event, BattleRoutine::CONTEXT_PORTRAIT, side ? 0x48 : 0, 0x100,
       person);
  Call(ds, event, BattleRoutine::ARMY_POSITION, x, 0x130);
  Call(ds, event, BattleRoutine::TEXT_COLOR, NAME_COLOR[side ? 1 : 0]);
  Call(ds, event, BattleRoutine::CURSOR_PRINT,
       Near(data, data.battle_commander_format), name);
  Call(ds, event, BattleRoutine::ARMY_POSITION, x, 0x140);
  Call(ds, event, BattleRoutine::TEXT_COLOR, TEXT_WHITE);
  Call(ds, event, BattleRoutine::UI_TEXT, person);
  Call(ds, event, BattleRoutine::CONTEXT_RESTORE);
  data.battle_panel_shown[side] = 1;
}

// frame: the caller's frame; the panel runs on the frame below it.
void opeu_battle_commander_wait(uint8_t* ds, BattleEvent event, uint16_t text,
                                uint8_t side, uint8_t value, uint16_t frame) {
  opeu_battle_commander_panel(ds, event, side, 1, Local(frame, CALLER_FRAME));
  Call(ds, event, BattleRoutine::CURSOR_PRINT,
       Near(Data(ds), Data(ds).battle_wait_format), text, value + '0');
}

void opeu_battle_commander_message(uint8_t* ds, BattleEvent event,
                                   uint16_t text, uint8_t side, uint8_t value,
                                   uint16_t frame) {
  opeu_battle_commander_panel(ds, event, side, 0, Local(frame, CALLER_FRAME));
  Call(ds, event, BattleRoutine::CURSOR_PRINT,
       Near(Data(ds), Data(ds).battle_prompt_format), text, value + '0');
}

void opeu_battle_element_destroy(uint8_t* ds, BattleEvent event,
                                 uint16_t element) {
  auto& data{ Data(ds) };
  Call(ds, event, BattleRoutine::BATTLE_CLEAR_CELL, PositionOf(data, element).x,
       PositionOf(data, element).y);
  auto const unit{ Call(ds, event, BattleRoutine::BATTLE_ELEMENT_UNIT, element,
                        0) };
  bool const selected{ (Record<Element>(data, element).flags &
                        Bits(ElementFlag::SELECTED)) != 0 };
  Call(ds, event, BattleRoutine::BATTLE_REMOVE_UNIT, unit, element);
  auto& record{ Record<Unit>(data, unit) };
  if (!(record.flags & Bits(UnitFlag::READY))) {
    record.resources[1] = 0;
    record.resources[0] = 0;
    record.resources[2] = 0;
    Call(ds, event, BattleRoutine::BATTLE_REMOVE_LIST, unit);
    return;
  }
  if (selected) {
    data.battle_lost_element = element;
    data.battle_lost_unit    = unit;
    Call(ds, event, BattleRoutine::BATTLE_HQ_LOST);
    data.battle_lost_element = 0;
    data.battle_lost_unit    = 0;
  }
  if (Call(ds, event, BattleRoutine::BATTLE_UNIT_ACTIVE, unit))
    return;
  auto const garrison{ Near(data, data.battle_garrison) };
  if (static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_UNIT_DIRECTION,
                                unit)) == BATTLE_NOT_ADJACENT &&
      unit != garrison)
    data.battle_direction = data.battle_exit_direction[0];
  if (unit == garrison) {
    auto&   objective{ Record<Objective>(
        data, Call(ds, event, BattleRoutine::FIND_OBJECTIVE_AT, data.battle_x,
                   data.battle_y)) };
    uint8_t gone{ 0 };
    for (auto& other : Elements(data, record))
      if ((other.flags & Bits(ElementFlag::READY)) &&
          (PositionOf(data, Near(data, other)).x & BATTLE_LEFT))
        ++gone;
    auto const percent{ Call(ds, event, BattleRoutine::RAND_BOUNDED,
                             gone * 10 + 41) };
    Reduce(ds, event, objective.resources[1], percent);
    Reduce(ds, event, objective.resources[2], percent);
    Reduce(ds, event, objective.resources[0], percent);
    for (auto& item : objective.items)
      if (item.quantity)
        item.quantity = 0;
  }
  Call(ds, event, BattleRoutine::BATTLE_REMOVE_LIST, unit);
}

// The services of the commander modules (this one and map/btlloss): the string copy, the unit removal and the ranged
// fire take their two pointer words BX before AX.
uint16_t svc_battle_commander(uint8_t* ds, uint32_t address,
                              const uint16_t* args, unsigned count) {
  static unsigned char const swap[]{ 1, 0 };
  switch (static_cast<BattleRoutine>(address)) {
    case BattleRoutine::STRING_COPY:
    case BattleRoutine::BATTLE_REMOVE_UNIT:
    case BattleRoutine::BATTLE_RANGED_FIRE:
      return btl_reorder(ds, address, args, count, swap, 2);
    default: return port_service(ds, address, args, count);
  }
}

uint16_t svc_btlcmdr(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count) {
  return svc_battle_commander(ds, service, args, count);
}

auto port_routine_opeu_battle_commander_message(std::uint8_t*        ds,
                                                std::uint16_t const* args,
                                                unsigned             count)
    -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) {
    opeu_battle_commander_message(ds, svc_btlcmdr, Arg(args, count, 2),
                                  static_cast<uint8_t>(Arg(args, count, 0)),
                                  static_cast<uint8_t>(Arg(args, count, 1)),
                                  BattleFrame(sp));
  });
  return 0;
}

auto port_routine_opeu_battle_commander_panel(std::uint8_t*        ds,
                                              std::uint16_t const* args,
                                              unsigned count) -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) {
    opeu_battle_commander_panel(ds, svc_btlcmdr,
                                static_cast<uint8_t>(Arg(args, count, 0)),
                                Arg(args, count, 1), BattleFrame(sp));
  });
  return 0;
}

auto port_routine_opeu_battle_commander_wait(std::uint8_t*        ds,
                                             std::uint16_t const* args,
                                             unsigned count) -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) {
    opeu_battle_commander_wait(ds, svc_btlcmdr, Arg(args, count, 2),
                               static_cast<uint8_t>(Arg(args, count, 0)),
                               static_cast<uint8_t>(Arg(args, count, 1)),
                               BattleFrame(sp));
  });
  return 0;
}

auto port_routine_opeu_battle_info_clear(std::uint8_t* ds, std::uint16_t const*,
                                         unsigned) -> std::uint32_t {
  opeu_battle_info_clear(ds, svc_btlcmdr);
  return 0;
}

auto port_routine_opeu_battle_name_clear_left(std::uint8_t* ds,
                                              std::uint16_t const*, unsigned)
    -> std::uint32_t {
  opeu_battle_name_clear_left(ds, svc_btlcmdr);
  return 0;
}

auto port_routine_opeu_battle_name_clear_right(std::uint8_t* ds,
                                               std::uint16_t const*, unsigned)
    -> std::uint32_t {
  opeu_battle_name_clear_right(ds, svc_btlcmdr);
  return 0;
}

auto port_routine_opeu_battle_panel_begin(std::uint8_t* ds,
                                          std::uint16_t const*, unsigned)
    -> std::uint32_t {
  opeu_battle_panel_begin(ds, svc_btlcmdr);
  return 0;
}

auto port_routine_opeu_battle_panel_side0(std::uint8_t* ds,
                                          std::uint16_t const*, unsigned)
    -> std::uint32_t {
  opeu_battle_panel_side0(ds, svc_btlcmdr);
  return 0;
}

auto port_routine_opeu_battle_panel_side1(std::uint8_t* ds,
                                          std::uint16_t const*, unsigned)
    -> std::uint32_t {
  opeu_battle_panel_side1(ds, svc_btlcmdr);
  return 0;
}

auto port_routine_opeu_battle_restore_tile(std::uint8_t*        ds,
                                           std::uint16_t const* args,
                                           unsigned count) -> std::uint32_t {
  opeu_battle_restore_tile(ds, svc_btlcmdr,
                           static_cast<uint8_t>(Arg(args, count, 0)),
                           static_cast<uint8_t>(Arg(args, count, 1)));
  return 0;
}
