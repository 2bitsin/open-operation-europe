// The cities views: lists of the own and the enemy side's cities, each city's stocks, and the menu that picks a side.
#include "core/vwcity.hpp"
#include "common/events.hpp"
#include "types.hpp"
#include "units/vwlist.hpp"

#include "common/data-segment.hpp"
#include "common/view-service.hpp"
#include "core/vwarms.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t CITY_PROMPT{ offsetof(DataSegment, city_prompt) };
constexpr std::uint16_t CITY_FORMAT{ offsetof(DataSegment, city_format) };
constexpr std::uint16_t CITY_NONE_OWN{ offsetof(DataSegment, city_none_own) };
constexpr std::uint16_t CITY_NONE_ENEMY{ offsetof(DataSegment, city_none_enemy) };
constexpr std::uint16_t CITY_SIDE_PROMPT{ offsetof(DataSegment, city_side_prompt) };
constexpr std::uint16_t CITY_SIDE_MENU{ offsetof(DataSegment, city_side_menu) };
constexpr std::uint16_t LIST_WINDOW{ offsetof(DataSegment, list_window) };

// The city list's locals whose addresses the services receive, as offsets in its reserve.
constexpr std::uint16_t TITLE_AT{ 0x00 };        // the stock list's title
constexpr std::uint16_t SAVED_WINDOW_AT{ 0x64 }; // the list window state saved around the stock list
constexpr std::uint16_t CITY_LIST_LOCALS{ 0x6e };

// Lists the side's cities and shows them; tells when the side holds none.
void ViewSideCities(std::uint8_t* ds, HostEvent event, std::uint8_t side, std::uint16_t none) {
  auto&      data{ Data(ds) };
  auto const count{ opeu_list_side_objectives(ds, event, side) };
  if (count == 0) {
    Call(ds, event, ViewService::CONTROL_MESSAGE, none, data.context_record3);
    return;
  }
  opeu_view_city_list(ds, event, count);
}

} // namespace

// Picks a listed city and flashes it; a city that holds stocks has them listed, the city list's window state kept
// aside meanwhile.
void opeu_view_city_list(uint8_t ds[65536], HostEvent event, uint8_t count) {
  ServiceLocals const locals{ CITY_LIST_LOCALS };
  auto&               data{ Data(ds) };
  auto const          saved_window{ locals.At(SAVED_WINDOW_AT) };
  auto const          title{ locals.At(TITLE_AT) };
  data.city_dialog.rows = static_cast<uint8_t>(count + 1);
  SetRowCheck(ds, event, data.city_dialog, RowCheck::OBJECTIVE);
  for (std::uint16_t height{ 7 }; Call(ds, event, ViewService::CONTROL_PANEL), Call(ds, event, ViewService::CONTROL_MESSAGE, CITY_PROMPT, data.context_record3),
                                  Call(ds, event, ViewService::LIST_DIALOG, Near(data, data.city_dialog), 1, height);
       height = 6) {
    auto const city{ data.view_objectives[data.list_row] };
    Call(ds, event, ViewService::UNIT_FLASH, city, 4);
    data.stock_dialog.rows = static_cast<uint8_t>(opeu_list_objective_stocks(ds, event, city) + 1);
    if (data.stock_dialog.rows > 1) {
      SetRowCheck(ds, event, data.stock_dialog, RowCheck::NONE_3);
      Call(ds, event, ViewService::CONTROL_PANEL);
      Call(ds, event, ViewService::REQUEST_FORMAT, title, CITY_FORMAT, city);
      Call(ds, event, ViewService::CONTROL_MESSAGE, title, data.context_record3);
      Call(ds, event, ViewService::COPY_BYTES, LIST_WINDOW, saved_window, LIST_WINDOW_STATE);
      Call(ds, event, ViewService::LIST_DIALOG, Near(data, data.stock_dialog), 1, 7);
      Call(ds, event, ViewService::COPY_BYTES, saved_window, LIST_WINDOW, LIST_WINDOW_STATE);
    }
  }
}

// The own side's cities.
void opeu_view_own_cities(uint8_t ds[65536], HostEvent event) {
  ViewSideCities(ds, event, Data(ds).current_side, CITY_NONE_OWN);
}

// The enemy side's cities.
void opeu_view_enemy_cities(uint8_t ds[65536], HostEvent event) {
  auto const enemy{ static_cast<std::uint8_t>(Call(ds, event, ViewService::OPPOSITE_SIDE, Data(ds).current_side)) };
  ViewSideCities(ds, event, enemy, CITY_NONE_ENEMY);
}

// The cities menu (own, enemy) until cancelled.
void opeu_view_cities_menu(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  for (std::uint16_t item; (item = Call(ds, event, ViewService::DELAY_DIALOG, CITY_SIDE_PROMPT, 2, data.context_record2, CITY_SIDE_MENU)) != MENU_CANCELLED;)
    RunAction(ds, event, data.city_actions, item);
}

uint16_t svc_vwcity(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_view(ds, "vwcity", id, args, count);
}

auto port_routine_opeu_view_cities_menu(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_cities_menu(ds, svc_vwcity);
  return 0;
}

auto port_routine_opeu_view_enemy_cities(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_enemy_cities(ds, svc_vwcity);
  return 0;
}

auto port_routine_opeu_view_own_cities(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_own_cities(ds, svc_vwcity);
  return 0;
}
