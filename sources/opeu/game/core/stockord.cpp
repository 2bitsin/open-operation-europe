// The city stockpile order (command action 2). The player picks one of the side's cities and sets how much fuel,
// ammunition and food it is to keep, beside the garrison's capacities and the city's stocks; refusing a question or
// the final confirmation restores the orders the city had.
#include "core/stockord.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "io/listdlg.hpp"
#include "map/mapunit.hpp"
#include "map/objeleg.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/resagg.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t STOCK_LIMIT{ 60000 };          // the largest stockpile order
constexpr std::uint16_t CITY_ENABLED_AT{ 0x4e };       // opeu_stock_city_enabled in this module's code segment
constexpr std::uint16_t STOCK_ORDER_SEGMENT{ 0x1d1e }; // this module's code segment
constexpr std::uint8_t  FLASH_TICKS{ 4 };              // how long the picked city flashes
constexpr std::uint8_t  FIRST_HEIGHT{ 7 };             // the list's rows the first time and after a city's orders
constexpr std::uint8_t  LATER_HEIGHT{ 6 };

enum Resource {
  AMMUNITION,
  FUEL,
  FOOD
};

} // namespace

// Lists the side's cities in view_objectives; returns their count.
uint8_t opeu_stock_cities_fill(Game& game) {
  auto&   data{ Data(game) };
  uint8_t count{ 0 };
  for (auto const& objective : data.objectives) {
    auto const near{ Near(data, objective) };
    if (objective.active != 0 && static_cast<std::uint8_t>(opeu_objective_side(game, near)) == data.current_side) data.view_objectives[count++] = near;
  }
  return count;
}

// The list's predicate: whether the city on the selected row may take orders.
uint8_t opeu_stock_city_enabled(Game& game) {
  auto const& data{ Data(game) };
  return opeu_objective_eligible(game, data.view_objectives[data.list_row]) != 0;
}

// The stockpile panel: per resource the garrison's capacity, the city's stock and the ordered stockpile. The
// capacities service writes the capacities into the routine's reserve.
void opeu_stock_show(Game& game, uint16_t objective) {
  ServiceLocals const locals{ 6 };
  auto&               data{ Data(game) };
  auto const          fuel{ locals.At(0) };
  auto const          food{ locals.At(2) };
  auto const          ammo{ locals.At(4) };
  auto const&         city{ Record<Objective>(data, objective) };
  opeu_objective_capacities(game, objective, port_ds_word_ptr(game, ammo), port_ds_word_ptr(game, fuel), port_ds_word_ptr(game, food));
  opeu_info_panel_begin(game, static_cast<const DialogHost*>(port_dialogs_host()));
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), offsetof(DataSegment, stock_header));
  opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(offsetof(DataSegment, stock_fuel_line)), static_cast<std::uint16_t>(Word(game, fuel)),
                                                        static_cast<std::uint16_t>(city.resources[FUEL]), static_cast<std::uint16_t>(city.reserves[FUEL]) });
  opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(offsetof(DataSegment, stock_ammo_line)), static_cast<std::uint16_t>(Word(game, ammo)),
                                                        static_cast<std::uint16_t>(city.resources[AMMUNITION]), static_cast<std::uint16_t>(city.reserves[AMMUNITION]) });
  opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(offsetof(DataSegment, stock_food_line)), static_cast<std::uint16_t>(Word(game, food)),
                                                        static_cast<std::uint16_t>(city.resources[FOOD]), static_cast<std::uint16_t>(city.reserves[FOOD]) });
}

// The panel under the city's name.
void opeu_stock_show_titled(Game& game, uint16_t objective) {
  opeu_army_title(game, static_cast<const DialogHost*>(port_dialogs_host()), objective);
  opeu_stock_show(game, objective);
}

// Asks how much of one resource to store (0..60000); MENU_CANCELLED when refused.
uint16_t opeu_stock_ask(Game& game, uint16_t what) {
  ServiceLocals const locals{ 0x64 }; // the question the format service writes
  auto const&         data{ Data(game) };
  auto const          question{ locals.At(0) };
  opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(question),
                                                          static_cast<std::uint16_t>(data.input_mode ? offsetof(DataSegment, stock_question_short) : offsetof(DataSegment, stock_question)),
                                                          static_cast<std::uint16_t>(what) });
  return opeu_control_choose(game, static_cast<const DialogHost*>(port_dialogs_host()), question, 0, STOCK_LIMIT, 1, data.context_record2, 0);
}

// Edits the city's three stockpile orders: fuel, ammunition, food, then the confirmation.
void opeu_stock_edit(Game& game, uint16_t objective) {
  auto&               data{ Data(game) };
  auto&               city{ Record<Objective>(data, objective) };
  std::uint16_t const before[]{ city.reserves[AMMUNITION], city.reserves[FUEL], city.reserves[FOOD] };
  auto const          ask{ [&](Resource resource, std::uint16_t what) { return (city.reserves[resource] = opeu_stock_ask(game, what)) != MENU_CANCELLED; } };
  opeu_stock_show(game, objective);
  if (ask(FUEL, offsetof(DataSegment, stock_fuel))) {
    opeu_stock_show_titled(game, objective);
    if (ask(AMMUNITION, offsetof(DataSegment, stock_ammo))) {
      opeu_stock_show(game, objective);
      if (ask(FOOD, offsetof(DataSegment, stock_food))) {
        opeu_stock_show(game, objective);
        if (opeu_confirm_ok(game, static_cast<const DialogHost*>(port_dialogs_host()), data.context_record2)) return;
      }
    }
  }
  for (auto const resource : { AMMUNITION, FUEL, FOOD }) city.reserves[resource] = before[resource];
  opeu_daily_context(game, svc_context, static_cast<std::uint8_t>(data.current_side));
}

// Command action 2: the list of the side's cities until the player leaves it. segment is this module's code segment,
// for the list's predicate.
void opeu_turn_action_2(Game& game, uint16_t segment) {
  auto& data{ Data(game) };
  auto& list{ data.city_dialog };
  list.rows = static_cast<std::uint8_t>(opeu_stock_cities_fill(game) + 1);
  if (list.rows == 1) {
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, stock_no_cities), data.context_record3);
    return;
  }
  list.select = { CITY_ENABLED_AT, segment };
  auto height{ FIRST_HEIGHT };
  for (;;) {
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, stock_select_city), data.context_record2);
    opeu_title_panel_begin(game, static_cast<const DialogHost*>(port_dialogs_host()));
    if (!opeu_list_dialog(game, svc_listdlg, offsetof(DataSegment, city_dialog), static_cast<std::uint8_t>(1), static_cast<std::uint8_t>(height))) break;
    auto const objective{ data.view_objectives[data.list_row] };
    opeu_unit_flash(game, objective, static_cast<uint8_t>(FLASH_TICKS));
    opeu_stock_edit(game, objective);
    height = LATER_HEIGHT;
  }
}

auto port_routine_opeu_stock_city_enabled(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_stock_city_enabled(game);
}

auto port_routine_opeu_turn_action_2(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_turn_action_2(game, STOCK_ORDER_SEGMENT);
  return 0;
}
