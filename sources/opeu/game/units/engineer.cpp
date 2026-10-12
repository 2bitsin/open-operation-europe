// The engineering orders: laying and clearing mines, building a bridge and repairing the arms of an objective or a
// regiment. Each handler takes the current regiment's order destination and needs the regiment next to it; mines and
// bridges lower the order's progress by a skill roll (the general's leadership, the regiment's averages, its engineers
// and a random term) and, when the progress runs out, report, change the map cell and show the day's picture whose
// finish routine ends the pending wait. A repair moves damaged arms back into service, up to the type's capacity, with
// engineers' strength / 10 + 1 per turn.
#include "units/engineer.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/order-service.hpp"
#include "core/limits.hpp"
#include "types.hpp"
#include "units/order-handler.hpp"

#include "core/dialogs.hpp"
#include "core/objrank.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "glue.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/maperase.hpp"
#include "map/movesup.hpp"
#include "map/movetgt.hpp"
#include "map/moveunit.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include "units/unitpers.hpp"
#include "units/value.hpp"

#include <utility>

using namespace opeu::game;

namespace {
// The calls the engineering handlers make through their event (served by svc_engineer).
enum Service : std::uint16_t {
  UNIT_PERSON           = 0,  // (unit) -> the regiment's general
  RAND_BOUNDED          = 1,  // (bound)
  UNIT_QUANTITY         = 2,  // (unit, kind, &count) -> strength of the regiment's elements of the kind
  UNIT_AVERAGE10        = 3,  // (unit)
  UNIT_AVERAGE11        = 4,  // (unit)
  ORDER_DESTINATION     = 5,  // (unit, &x, &y)
  CELL_MARKED           = 6,  // (x, y, side) -> nonzero when the side mined the cell
  MOVE_ADJACENT         = 7,  // (x, y) -> nonzero when the regiment is next to the cell
  LEVEL_LOWER           = 8,  // (order, amount) -> nonzero when the progress ran out
  CELL_SIDE             = 9,  // (x, y) -> the side holding the cell, 0xff for none
  UNIT_REPORT           = 10, // (unit, text)
  MARK_CELL             = 11, // (x, y, side): mine the cell
  MAP_ERASE             = 12, // (x, y)
  DAILY_SHOW            = 13, // (picture, side, finish offset, finish segment)
  RESTORE_ORDER         = 14, // (unit)
  UNMARK_CELL           = 15, // (x, y, side)
  MAP_CODE              = 16, // (x, y) -> terrain code
  MAP_MARK_BRIDGE       = 17, // (x, y)
  MOVE_OBJECTIVE_TARGET = 18, // (&x, &y, side) -> the objective of the order
  MOVE_UNIT_TARGET      = 19, // (&x, &y) -> the regiment of the order
  PENDING_FINISH        = 20, // (value)
  SERVICES              = 21,
};

// The pictures of the day's show and the link-table addresses of the routines that end it.
constexpr std::uint16_t ENGINEER_SEGMENT{ 0x319e };
enum Picture : std::uint16_t {
  MINES_LAID_PICTURE    = 0x191,
  MINES_REMOVED_PICTURE = 0x68,
  BRIDGE_PICTURE        = 0x1a2,
  ARMS_PICTURE          = 0x1b4,
};
enum Finish : std::uint16_t {
  MINES_LAID_FINISH    = 0x00a0,
  MINES_REMOVED_FINISH = 0x016a,
  BRIDGE_BUILT_FINISH  = 0x02b8,
  ARMS_REPAIRED_FINISH = 0x03ba,
};

// The frame locals the handlers hand to their services, as offsets below the frame pointer.
enum FrameLocal : std::uint16_t {
  ENGINEER_COUNT = 2, // the skill roll's unit_quantity count byte
  DESTINATION_X  = 2,
  DESTINATION_Y  = 4,
  REPAIR_BUDGET  = 8,
  TARGET_X       = 10,
  TARGET_Y       = 12,
  CREW_COUNT     = 14, // the regiment repair's count byte
  CITY_CREWS     = 16, // the objective repair's count byte
  SKILL_FRAME    = 10, // the skill roll's frame lies this far below the handler's
};

auto Send(std::uint8_t* ds, HostEvent event, Service service, auto... words) -> std::uint16_t {
  return Call(ds, event, std::to_underlying(service), words...);
}

// The engineering skill roll, computed in 16-bit words of which the low byte is kept. The services are called in the
// order general, random bound, engineer count, average10, average11.
auto SkillRoll(std::uint8_t* ds, HostEvent event, std::uint16_t frame, std::uint16_t person, std::uint16_t bound,
               std::uint16_t leadership_divisor, std::uint16_t engineer_divisor, std::uint16_t bonus) -> std::uint8_t {
  auto&               data{ Data(ds) };
  std::uint16_t const random{ Send(ds, event, RAND_BOUNDED, bound) };
  std::uint16_t const engineers{ Send(ds, event, UNIT_QUANTITY, data.current_unit_ptr, Bits(Category::ENGINEERS),
                                      Local(frame, ENGINEER_COUNT)) };
  std::uint16_t const average10{ Send(ds, event, UNIT_AVERAGE10, data.current_unit_ptr) };
  std::uint16_t const average11{ Send(ds, event, UNIT_AVERAGE11, data.current_unit_ptr) };
  return static_cast<std::uint8_t>(average11 / 19
                                   - static_cast<std::uint16_t>(Record<Person>(data, person).leadership + average10)
                                         / leadership_divisor
                                   - engineers / engineer_divisor + random + bonus);
}

// Lowers the current order's progress by the roll; nonzero when it runs out. The regiment is read after the roll.
auto LowerProgress(std::uint8_t* ds, HostEvent event, std::uint8_t roll) -> std::uint16_t {
  auto& data{ Data(ds) };
  return Send(ds, event, LEVEL_LOWER, Near(data, Record<Unit>(data, data.current_unit_ptr).orders[0]), roll);
}

auto Show(std::uint8_t* ds, HostEvent event, Picture picture, Finish finish) -> void {
  Send(ds, event, DAILY_SHOW, picture, Data(ds).current_side, finish, ENGINEER_SEGMENT);
}

auto Report(std::uint8_t* ds, HostEvent event, auto const& text) -> void {
  auto& data{ Data(ds) };
  Send(ds, event, UNIT_REPORT, data.current_unit_ptr, Near(data, text));
}

auto RestoreOrder(std::uint8_t* ds, HostEvent event) -> void {
  Send(ds, event, RESTORE_ORDER, Data(ds).current_unit_ptr);
}
} // namespace

uint8_t opeu_mine_skill(uint8_t ds[65536], HostEvent event, uint16_t frame) {
  auto const person{ Send(ds, event, UNIT_PERSON, Data(ds).current_unit_ptr) };
  return SkillRoll(ds, event, frame, person, RollBound(Data(ds)), 45, 96, 8);
}

void opeu_mines_laid_finish(uint8_t ds[65536], HostEvent event) {
  Send(ds, event, PENDING_FINISH, SHOW_DONE);
}

// Lays mines at the destination cell unless the side has mined it already or the regiment stands on it.
uint16_t opeu_engineer_lay_mines(uint8_t ds[65536], HostEvent event, uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const at_x{ Local(frame, DESTINATION_X) };
  auto const at_y{ Local(frame, DESTINATION_Y) };
  Send(ds, event, ORDER_DESTINATION, data.current_unit_ptr, at_x, at_y);
  auto const  x{ Byte(ds, at_x) };
  auto const  y{ Byte(ds, at_y) };
  auto const& regiment{ Record<Unit>(data, data.current_unit_ptr) };
  if (!Send(ds, event, CELL_MARKED, x, y, data.current_side) && (regiment.x != x || regiment.y != y)) {
    if (!Send(ds, event, MOVE_ADJACENT, x, y)) return 0;
    if (LowerProgress(ds, event, opeu_mine_skill(ds, event, Local(frame, SKILL_FRAME)))
        && static_cast<std::uint8_t>(Send(ds, event, CELL_SIDE, x, y)) == NO_SIDE) {
      Report(ds, event, data.mines_laid_text);
      Send(ds, event, MARK_CELL, x, y, data.current_side);
      Send(ds, event, MAP_ERASE, x, y);
      Show(ds, event, MINES_LAID_PICTURE, MINES_LAID_FINISH);
      RestoreOrder(ds, event);
    }
    return 1;
  }
  RestoreOrder(ds, event);
  return 1;
}

void opeu_mines_removed_finish(uint8_t ds[65536], HostEvent event) {
  Send(ds, event, PENDING_FINISH, SHOW_DONE);
}

// Clears the mines of both sides at the destination cell.
uint16_t opeu_engineer_remove_mines(uint8_t ds[65536], HostEvent event, uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const at_x{ Local(frame, DESTINATION_X) };
  auto const at_y{ Local(frame, DESTINATION_Y) };
  Send(ds, event, ORDER_DESTINATION, data.current_unit_ptr, at_x, at_y);
  auto const x{ Byte(ds, at_x) };
  auto const y{ Byte(ds, at_y) };
  if (Send(ds, event, CELL_MARKED, x, y, data.current_side)) {
    if (!Send(ds, event, MOVE_ADJACENT, x, y)) return 0;
    if (LowerProgress(ds, event, opeu_mine_skill(ds, event, Local(frame, SKILL_FRAME)))) {
      Report(ds, event, data.mines_removed_text);
      Send(ds, event, UNMARK_CELL, x, y, 0);
      Send(ds, event, UNMARK_CELL, x, y, 1);
      Send(ds, event, MAP_ERASE, x, y);
      Show(ds, event, MINES_REMOVED_PICTURE, MINES_REMOVED_FINISH);
      RestoreOrder(ds, event);
    }
    return 1;
  }
  RestoreOrder(ds, event);
  return 1;
}

uint8_t opeu_bridge_skill(uint8_t ds[65536], HostEvent event, uint16_t frame) {
  auto const person{ Send(ds, event, UNIT_PERSON, Data(ds).current_unit_ptr) };
  return SkillRoll(ds, event, frame, person, RollBound(Data(ds)), 35, 80, 10);
}

void opeu_bridge_built_finish(uint8_t ds[65536], HostEvent event) {
  Send(ds, event, PENDING_FINISH, SHOW_DONE);
}

// Builds a bridge over the river at the destination cell.
uint16_t opeu_engineer_build_bridge(uint8_t ds[65536], HostEvent event, uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const at_x{ Local(frame, DESTINATION_X) };
  auto const at_y{ Local(frame, DESTINATION_Y) };
  Send(ds, event, ORDER_DESTINATION, data.current_unit_ptr, at_x, at_y);
  auto const x{ Byte(ds, at_x) };
  auto const y{ Byte(ds, at_y) };
  if (static_cast<Terrain>(static_cast<std::uint8_t>(Send(ds, event, MAP_CODE, x, y))) == Terrain::RIVER) {
    if (!Send(ds, event, MOVE_ADJACENT, x, y)) return 0;
    if (LowerProgress(ds, event, opeu_bridge_skill(ds, event, Local(frame, SKILL_FRAME)))
        && static_cast<std::uint8_t>(Send(ds, event, CELL_SIDE, x, y)) == NO_SIDE) {
      Report(ds, event, data.bridge_built_text);
      Send(ds, event, MAP_MARK_BRIDGE, x, y);
      Send(ds, event, MAP_ERASE, x, y);
      Show(ds, event, BRIDGE_PICTURE, BRIDGE_BUILT_FINISH);
      RestoreOrder(ds, event);
    }
    return 1;
  }
  RestoreOrder(ds, event);
  return 1;
}

// Moves damaged arms of the item (an objective's item or an element's type, quantity and damage) back into service:
// at most the budget at `amount`, the damaged count, and what the type's capacity leaves room for. Nonzero when the
// item had damage and budget left.
uint16_t opeu_arms_repair(uint8_t ds[65536], uint16_t amount, uint16_t body) {
  auto&      data{ Data(ds) };
  auto&      item{ Record<Item>(data, body) };
  auto const budget{ Word(ds, amount) };
  if (!budget || !item.extra) return 0;
  auto const capacity{ data.request_capacities[data.element_types[item.type].category] };
  auto const repaired{ static_cast<std::uint16_t>(opeu_min16(opeu_min16(budget, item.extra) + item.quantity, capacity)
                                                  - item.quantity) };
  item.extra    -= repaired;
  item.quantity += repaired;
  SetWord(ds, amount, static_cast<std::uint16_t>(budget - repaired));
  return 1;
}

void opeu_arms_repaired_finish(uint8_t ds[65536], HostEvent event) {
  Send(ds, event, PENDING_FINISH, SHOW_DONE);
}

// Repairs the arms of the own objective at the destination; the order ends when nothing is left to repair.
uint16_t opeu_engineer_repair_objective(uint8_t ds[65536], HostEvent event, uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const crews{ Local(frame, CITY_CREWS) };
  if (!Send(ds, event, UNIT_QUANTITY, data.current_unit_ptr, Bits(Category::REPAIR), crews)) {
    RestoreOrder(ds, event);
    return 1;
  }
  auto const at_x{ Local(frame, TARGET_X) };
  auto const at_y{ Local(frame, TARGET_Y) };
  auto const objective{ Send(ds, event, MOVE_OBJECTIVE_TARGET, at_x, at_y, data.current_side) };
  if (!objective) return 1;
  if (!Send(ds, event, MOVE_ADJACENT, Byte(ds, at_x), Byte(ds, at_y))) return 0;
  Record<Unit>(data, data.current_unit_ptr).orders[0].progress = 0;
  auto const budget{ Local(frame, REPAIR_BUDGET) };
  SetWord(ds, budget,
          static_cast<std::uint16_t>(
              Send(ds, event, UNIT_QUANTITY, data.current_unit_ptr, Bits(Category::REPAIR), crews) / 10 + 1));
  bool repaired{ false };
  for (auto& item : Record<Objective>(data, objective).items)
    if (item.quantity && opeu_arms_repair(ds, budget, Near(data, item))) repaired = true;
  if (!repaired) {
    Report(ds, event, data.arms_repaired_text);
    Show(ds, event, ARMS_PICTURE, ARMS_REPAIRED_FINISH);
    RestoreOrder(ds, event);
  }
  return 1;
}

// Repairs the arms of the friendly regiment the order targets; the order ends when nothing is left to repair.
uint16_t opeu_engineer_repair_unit(uint8_t ds[65536], HostEvent event, uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const crews{ Local(frame, CREW_COUNT) };
  if (!Send(ds, event, UNIT_QUANTITY, data.current_unit_ptr, Bits(Category::REPAIR), crews)) {
    RestoreOrder(ds, event);
    return 1;
  }
  auto const at_x{ Local(frame, TARGET_X) };
  auto const at_y{ Local(frame, TARGET_Y) };
  auto const unit{ Send(ds, event, MOVE_UNIT_TARGET, at_x, at_y) };
  if (!unit) return 1;
  if (!Send(ds, event, MOVE_ADJACENT, Byte(ds, at_x), Byte(ds, at_y))) return 0;
  Record<Unit>(data, data.current_unit_ptr).orders[0].progress = 0;
  auto const budget{ Local(frame, REPAIR_BUDGET) };
  SetWord(ds, budget,
          static_cast<std::uint16_t>(
              Send(ds, event, UNIT_QUANTITY, data.current_unit_ptr, Bits(Category::REPAIR), crews) / 10 + 1));
  bool repaired{ false };
  for (auto element{ Record<Unit>(data, unit).first }; element; element = Record<Element>(data, element).next)
    if (opeu_arms_repair(ds, budget, Near(data, Record<Element>(data, element).type))) repaired = true;
  if (!repaired) {
    Report(ds, event, data.unit_arms_repaired_text);
    Show(ds, event, ARMS_PICTURE, ARMS_REPAIRED_FINISH);
    RestoreOrder(ds, event);
  }
  return 1;
}

// The routines behind the engineering handlers' calls. The calls they share with the other order handlers go
// through svc_order_handler.
uint16_t svc_engineer(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  // The order handler service of each engineering call; NONE for the calls served here.
  constexpr auto                                      NONE{ static_cast<OrderService>(0xff) };
  static constexpr std::array<OrderService, SERVICES> shared{
    OrderService::UNIT_PERSON,
    OrderService::RAND_BOUNDED,
    NONE,
    OrderService::UNIT_AVERAGE10,
    OrderService::UNIT_AVERAGE11,
    OrderService::UNIT_ORDER_DESTINATION,
    NONE,
    OrderService::MOVE_ADJACENT,
    OrderService::MOVE_LEVEL_LOWER,
    OrderService::CELL_SIDE,
    OrderService::UNIT_REPORT,
    NONE,
    OrderService::MAP_ERASE,
    OrderService::DAILY_SHOW,
    OrderService::RESTORE_UNIT_ORDER,
    NONE,
    OrderService::MAP_CODE,
    NONE,
    OrderService::MOVE_OBJECTIVE_TARGET,
    OrderService::MOVE_UNIT_TARGET,
    OrderService::PENDING_FINISH,
  };
  uint16_t r = 0;
  switch (id) {
  case UNIT_QUANTITY:
    r = opeu_unit_quantity(ds, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), PORT_DS(Arg(args, count, 2)));
    break;
  case CELL_MARKED:
    r = opeu_cell_marked(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2));
    break;
  case MARK_CELL:
    opeu_mark_cell(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2));
    break;
  case UNMARK_CELL:
    opeu_unmark_cell(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2));
    break;
  case MAP_MARK_BRIDGE: opeu_map_mark_10(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  default:
    if (id >= SERVICES || shared[id] == NONE || !svc_order_handler(ds, shared[id], args, count, &r))
      port_unhandled("engineer", id, count);
  }
  return r;
}

// The services the order handlers share (OrderService); 0 when the id is not one of them. Pointer arguments are DS
// offsets; byte arguments narrow.
int svc_order_handler(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count, uint16_t* result) {
  uint16_t r = 0;
  switch (id) {
  case OrderService::UNIT_AVERAGE11:     r = opeu_unit_average11(ds, Arg(args, count, 0)); break;
  case OrderService::RAND_BOUNDED:       r = port_rand_bounded(ds, Arg(args, count, 0)); break;
  case OrderService::RESTORE_UNIT_ORDER: opeu_restore_unit_order(ds, Arg(args, count, 0)); break;
  case OrderService::GRID_STEP:
    r = opeu_grid_step(PORT_DS(Arg(args, count, 0)), PORT_DS(Arg(args, count, 1)), (uint8_t)Arg(args, count, 2));
    break;
  case OrderService::FIND_UNIT_AT:
    r = opeu_find_unit_at(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1));
    break;
  case OrderService::UNIT_SIDE:     r = opeu_unit_side(Arg(args, count, 0)); break;
  case OrderService::UNIT_VALUE:    r = opeu_unit_value(ds, Arg(args, count, 0)); break;
  case OrderService::MOVE_SUPPLIED: r = opeu_move_supplied(ds); break;
  case OrderService::UNIT_REPORT:
    opeu_unit_report(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1));
    break;
  case OrderService::UNIT_SELECTION_ELIGIBLE: r = opeu_unit_selection_eligible(ds, Arg(args, count, 0)); break;
  case OrderService::REQUEST_FORMAT:          r = lf_request_format(ds, args, count); break;
  case OrderService::CONFIRM_MESSAGE:
    r = opeu_confirm_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1));
    break;
  case OrderService::DAILY_SHOW:
    opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1),
                    Arg32(args, count, 2));
    break;
  case OrderService::OPPOSITE_SIDE: r = opeu_opposite_side((uint8_t)Arg(args, count, 0)); break;
  case OrderService::MOVE_OBJECTIVE_TARGET:
    r = opeu_move_objective_target(ds, svc_movetgt, Arg(args, count, 0), Arg(args, count, 1),
                                   (uint8_t)Arg(args, count, 2));
    break;
  case OrderService::MOVE_ADJACENT:
    r = WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
      return opeu_move_adjacent(ds, svc_movetgt, static_cast<std::uint16_t>(sp - 2u), (uint8_t)Arg(args, count, 0),
                                (uint8_t)Arg(args, count, 1));
    });
    break;
  case OrderService::MOVE_LEVEL_LOWER:
    r = opeu_move_level_lower(ds, Arg(args, count, 0), (uint8_t)Arg(args, count, 1));
    break;
  case OrderService::CELL_SIDE:
    r = opeu_cell_side(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1));
    break;
  case OrderService::OBJECTIVE_QUANTITY:
    r = opeu_objective_quantity(ds, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), PORT_DS(Arg(args, count, 2)));
    break;
  case OrderService::MOVE_UNIT:
    WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
      opeu_move_unit(ds, svc_moveunit, static_cast<std::uint16_t>(sp - 2u), Arg(args, count, 0));
    });
    break;
  case OrderService::SIDE_GROUPS_RETARGET: opeu_side_groups_retarget(ds, (uint8_t)Arg(args, count, 0)); break;
  case OrderService::MOVE_UNIT_TARGET:
    r = WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
      return opeu_move_unit_target(ds, svc_movetgt, static_cast<std::uint16_t>(sp - 2u), Arg(args, count, 0),
                                   Arg(args, count, 1));
    });
    break;
  case OrderService::UNIT_NAME:   r = opeu_unit_name(ds, svc_unitname, Arg(args, count, 0), Arg(args, count, 1)); break;
  case OrderService::ABS16_BITS:  r = opeu_abs16_bits(Arg(args, count, 0)); break;
  case OrderService::UNIT_PERSON: r = opeu_unit_person(ds, Arg(args, count, 0)); break;
  case OrderService::UNIT_AVERAGE10: r = opeu_unit_average10(ds, Arg(args, count, 0)); break;
  case OrderService::UNIT_AVERAGE9:  r = opeu_unit_average9(ds, Arg(args, count, 0)); break;
  case OrderService::UNIT_ORDER_DESTINATION:
    r = opeu_unit_order_destination(ds, Arg(args, count, 0), PORT_DS(Arg(args, count, 1)),
                                    PORT_DS(Arg(args, count, 2)));
    break;
  case OrderService::MAP_CODE: r = opeu_map_code(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case OrderService::MAP_MARK5: opeu_map_mark_5(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case OrderService::MAP_ERASE:
    WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
      opeu_map_erase(ds, svc_maperase, static_cast<std::uint16_t>(sp - 2u), (uint8_t)Arg(args, count, 0),
                     (uint8_t)Arg(args, count, 1));
    });
    break;
  case OrderService::PENDING_FINISH: opeu_pending_finish(ds, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  case OrderService::WAIT_UNITS:     opeu_delay_units(ds, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  case OrderService::SOUND_CUE5:     opeu_sound_cue5(ds, svc_soundcue); break;
  default:                           return 0;
  }
  *result = r;
  return 1;
}

auto port_routine_opeu_arms_repaired_finish(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_arms_repaired_finish(ds, svc_engineer);
  return 0;
}

auto port_routine_opeu_bridge_built_finish(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_bridge_built_finish(ds, svc_engineer);
  return 0;
}

auto port_routine_opeu_engineer_build_bridge(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_engineer_build_bridge(ds, svc_engineer, static_cast<std::uint16_t>(sp - 2));
  });
}

auto port_routine_opeu_engineer_lay_mines(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_engineer_lay_mines(ds, svc_engineer, static_cast<std::uint16_t>(sp - 2));
  });
}

auto port_routine_opeu_engineer_remove_mines(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_engineer_remove_mines(ds, svc_engineer, static_cast<std::uint16_t>(sp - 2));
  });
}

auto port_routine_opeu_engineer_repair_objective(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_engineer_repair_objective(ds, svc_engineer, static_cast<std::uint16_t>(sp - 2));
  });
}

auto port_routine_opeu_engineer_repair_unit(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_engineer_repair_unit(ds, svc_engineer, static_cast<std::uint16_t>(sp - 2));
  });
}

auto port_routine_opeu_mines_laid_finish(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_mines_laid_finish(ds, svc_engineer);
  return 0;
}

auto port_routine_opeu_mines_removed_finish(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_mines_removed_finish(ds, svc_engineer);
  return 0;
}
