// The city stockpile order (command action 2). The player picks one of the side's cities and sets how much fuel,
// ammunition and food it is to keep, beside the garrison's capacities and the city's stocks; refusing a question or
// the final confirmation restores the orders the city had.
#include "types.hpp"
#include "core/stockord.hpp"
#include "types/events.hpp"

#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

using enum BattleRoutine;

constexpr std::uint16_t CANCELLED{ 0xffff };  // a refused question
constexpr std::uint16_t STOCK_LIMIT{ 60000 }; // the largest stockpile order
constexpr std::uint16_t CITY_ENABLED_AT{
  0x4e
}; // opeu_stock_city_enabled in this module's code segment
constexpr std::uint16_t STOCK_ORDER_SEGMENT{
  0x1d1e
};                                       // this module's code segment
constexpr std::uint8_t FLASH_TICKS{ 4 }; // how long the picked city flashes
constexpr std::uint8_t FIRST_HEIGHT{
  7
}; // the list's rows the first time and after a city's orders
constexpr std::uint8_t LATER_HEIGHT{ 6 };

enum Resource { AMMUNITION, FUEL, FOOD };

} // namespace

// Lists the side's cities in view_objectives; returns their count.
uint8_t opeu_stock_cities_fill(uint8_t* ds, BattleEvent event, uint16_t sp) {
  (void)sp;
  auto&   data{ Data(ds) };
  uint8_t count{ 0 };
  for (auto const& objective : data.objectives) {
    auto const near{ Near(data, objective) };
    if (objective.active != 0 &&
        static_cast<std::uint8_t>(Call(ds, event, OBJECTIVE_SIDE, near)) ==
            data.current_side)
      data.view_objectives[count++] = near;
  }
  return count;
}

// The list's predicate: whether the city on the selected row may take orders.
uint8_t opeu_stock_city_enabled(uint8_t* ds, BattleEvent event, uint16_t sp) {
  (void)sp;
  auto const& data{ Data(ds) };
  return Call(ds, event, OBJECTIVE_ELIGIBLE,
              data.view_objectives[data.list_row]) != 0;
}

// The stockpile panel: per resource the garrison's capacity, the city's stock and the ordered stockpile. The
// capacities land in frame locals.
void opeu_stock_show(uint8_t* ds, BattleEvent event, uint16_t sp,
                     uint16_t objective) {
  auto&       data{ Data(ds) };
  auto const  frame{ static_cast<std::uint16_t>(sp - 2) };
  auto const  ammo{ Local(frame, 2) };
  auto const  food{ Local(frame, 4) };
  auto const  fuel{ Local(frame, 6) };
  auto const& city{ Record<Objective>(data, objective) };
  Call(ds, event, OBJECTIVE_CAPACITIES, objective, ammo, fuel, food);
  Call(ds, event, INFO_PANEL_BEGIN);
  Call(ds, event, UI_TEXT, offsetof(DataSegment, stock_header));
  Call(ds, event, CURSOR_PRINT, offsetof(DataSegment, stock_fuel_line),
       Word(ds, fuel), city.resources[FUEL], city.reserves[FUEL]);
  Call(ds, event, CURSOR_PRINT, offsetof(DataSegment, stock_ammo_line),
       Word(ds, ammo), city.resources[AMMUNITION], city.reserves[AMMUNITION]);
  Call(ds, event, CURSOR_PRINT, offsetof(DataSegment, stock_food_line),
       Word(ds, food), city.resources[FOOD], city.reserves[FOOD]);
}

// The panel under the city's name.
void opeu_stock_show_titled(uint8_t* ds, BattleEvent event, uint16_t sp,
                            uint16_t objective) {
  Call(ds, event, ARMY_TITLE, objective);
  opeu_stock_show(ds, event, static_cast<std::uint16_t>(sp - 6), objective);
}

// Asks how much of one resource to store (0..60000); CANCELLED when refused.
uint16_t opeu_stock_ask(uint8_t* ds, BattleEvent event, uint16_t sp,
                        uint16_t what) {
  auto const& data{ Data(ds) };
  auto const  question{ Local(static_cast<std::uint16_t>(sp - 2), 0x64) };
  Call(ds, event, REQUEST_FORMAT, question,
       data.input_mode ? offsetof(DataSegment, stock_question_short)
                       : offsetof(DataSegment, stock_question),
       what);
  return Call(ds, event, CONTROL_CHOOSE, question, 0, STOCK_LIMIT, 1,
              data.context_record2, 0);
}

// Edits the city's three stockpile orders: fuel, ammunition, food, then the confirmation.
void opeu_stock_edit(uint8_t* ds, BattleEvent event, uint16_t sp,
                     uint16_t objective) {
  auto&               data{ Data(ds) };
  auto&               city{ Record<Objective>(data, objective) };
  std::uint16_t const before[]{ city.reserves[AMMUNITION], city.reserves[FUEL],
                                city.reserves[FOOD] };
  auto const          inner{ static_cast<std::uint16_t>(sp - 16) };
  auto const          ask{ [&](Resource resource, std::uint16_t what) {
    return (city.reserves[resource] = opeu_stock_ask(ds, event, inner, what)) !=
           CANCELLED;
  } };
  opeu_stock_show(ds, event, inner, objective);
  if (ask(FUEL, offsetof(DataSegment, stock_fuel))) {
    opeu_stock_show_titled(ds, event, inner, objective);
    if (ask(AMMUNITION, offsetof(DataSegment, stock_ammo))) {
      opeu_stock_show(ds, event, inner, objective);
      if (ask(FOOD, offsetof(DataSegment, stock_food))) {
        opeu_stock_show(ds, event, inner, objective);
        if (Call(ds, event, CONFIRM_OK, data.context_record2))
          return;
      }
    }
  }
  for (auto const resource : { AMMUNITION, FUEL, FOOD })
    city.reserves[resource] = before[resource];
  Call(ds, event, DAILY_CONTEXT, data.current_side);
}

// Command action 2: the list of the side's cities until the player leaves it. segment is this module's code segment,
// for the list's predicate.
void opeu_turn_action_2(uint8_t* ds, BattleEvent event, uint16_t sp,
                        uint16_t segment) {
  auto&      data{ Data(ds) };
  auto&      list{ data.city_dialog };
  auto const inner{ static_cast<std::uint16_t>(sp - 10) };
  list.rows =
      static_cast<std::uint8_t>(opeu_stock_cities_fill(ds, event, inner) + 1);
  if (list.rows == 1) {
    Call(ds, event, CONTROL_MESSAGE, offsetof(DataSegment, stock_no_cities),
         data.context_record3);
    return;
  }
  list.select = { CITY_ENABLED_AT, segment };
  auto height{ FIRST_HEIGHT };
  for (;;) {
    Call(ds, event, CONTROL_MESSAGE, offsetof(DataSegment, stock_select_city),
         data.context_record2);
    Call(ds, event, TITLE_PANEL_BEGIN);
    if (!Call(ds, event, LIST_DIALOG, offsetof(DataSegment, city_dialog), 1,
              height))
      break;
    auto const objective{ data.view_objectives[data.list_row] };
    Call(ds, event, UNIT_FLASH, objective, FLASH_TICKS);
    opeu_stock_edit(ds, event, inner, objective);
    height = LATER_HEIGHT;
  }
}

// The routines take their register arguments in AX, DX, BX order, so those with more than one register argument get
// their arguments reordered; the rest forward as they are.
uint16_t svc_stockord(uint8_t* ds, uint32_t service, const uint16_t* args,
                      unsigned int count) {
  static constexpr unsigned char capacities[]{
    1, 2, 0, 3
  }; // (objective, a, b, c) -> (a, b, objective, c)
  static constexpr unsigned char swap[]{ 1,
                                         0 }; // (record, ticks), (text, person)
  static constexpr unsigned char choose[]{
    1, 2, 0, 3, 4, 5
  }; // (text, first, last, ...) -> (first, last, text, ...)
  static constexpr unsigned char list_dialog[]{
    1, 2, 0
  }; // (window, limit, flags) -> (limit, flags, window)
  switch (static_cast<BattleRoutine>(service)) {
    case OBJECTIVE_CAPACITIES:
      return btl_reorder(ds, service, args, count, capacities, 4);
    case UNIT_FLASH:
    case CONTROL_MESSAGE: return btl_reorder(ds, service, args, count, swap, 2);
    case CONTROL_CHOOSE:
      return btl_reorder(ds, service, args, count, choose, 6);
    case LIST_DIALOG:
      return btl_reorder(ds, service, args, count, list_dialog, 3);
    default: return port_service(ds, service, args, count);
  }
}

auto port_routine_opeu_stock_city_enabled(std::uint8_t* ds,
                                          std::uint16_t const*, unsigned)
    -> std::uint32_t {
  return WithBattleStack([ds](std::uint16_t sp) -> std::uint32_t {
    return opeu_stock_city_enabled(ds, svc_stockord, sp);
  });
}

auto port_routine_opeu_turn_action_2(std::uint8_t* ds, std::uint16_t const*,
                                     unsigned) -> std::uint32_t {
  return WithBattleStack([ds](std::uint16_t sp) -> std::uint32_t {
    opeu_turn_action_2(ds, svc_stockord, sp, STOCK_ORDER_SEGMENT);
    return 0;
  });
}
