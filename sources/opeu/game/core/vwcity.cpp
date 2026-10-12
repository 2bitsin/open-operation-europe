// The cities views: lists of the own and the enemy side's cities, each city's stocks, and the menu that picks a side.
#include "types.hpp"
#include "core/vwcity.hpp"
#include "units/vwlist.hpp"
#include "common/events.hpp"

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
constexpr std::uint16_t CITY_NONE_ENEMY{ offsetof(DataSegment,
                                                  city_none_enemy) };
constexpr std::uint16_t CITY_SIDE_PROMPT{ offsetof(DataSegment,
                                                   city_side_prompt) };
constexpr std::uint16_t CITY_SIDE_MENU{ offsetof(DataSegment, city_side_menu) };
constexpr std::uint16_t LIST_WINDOW{ offsetof(DataSegment, list_window) };

// Frame locals of the city list whose addresses the services receive.
constexpr std::uint16_t SAVED_WINDOW_LOCAL{
  0xa
}; // the list window state saved around the stock list
constexpr std::uint16_t TITLE_LOCAL{ 0x6e }; // the stock list's title

// Lists the side's cities and shows them; tells when the side holds none.
void ViewSideCities(std::uint8_t* ds, HostEvent event, std::uint16_t sp,
                    std::uint8_t side, std::uint16_t none) {
  auto&      data{ Data(ds) };
  auto const count{ opeu_list_side_objectives(ds, event, CalleeStack(sp, 2),
                                              side) };
  if (count == 0) {
    Call(ds, event, ViewService::CONTROL_MESSAGE, none, data.context_record3);
    return;
  }
  opeu_view_city_list(ds, event, CalleeStack(sp, 2), count);
}

} // namespace

// Picks a listed city and flashes it; a city that holds stocks has them listed, the city list's window state kept
// aside meanwhile.
void opeu_view_city_list(uint8_t ds[65536], HostEvent event, uint16_t sp,
                         uint8_t count) {
  auto&      data{ Data(ds) };
  auto const saved_window{ FrameLocal(sp, SAVED_WINDOW_LOCAL) };
  auto const title{ FrameLocal(sp, TITLE_LOCAL) };
  data.city_dialog.rows = static_cast<uint8_t>(count + 1);
  SetRowCheck(ds, event, data.city_dialog, RowCheck::OBJECTIVE);
  for (std::uint16_t height{ 7 }; Call(ds, event, ViewService::CONTROL_PANEL),
                                  Call(ds, event, ViewService::CONTROL_MESSAGE,
                                       CITY_PROMPT, data.context_record3),
                                  Call(ds, event, ViewService::LIST_DIALOG,
                                       Near(data, data.city_dialog), 1, height);
       height = 6) {
    auto const city{ data.view_objectives[data.list_row] };
    Call(ds, event, ViewService::UNIT_FLASH, city, 4);
    data.stock_dialog.rows = static_cast<uint8_t>(
        opeu_list_objective_stocks(ds, event, CalleeStack(sp, 0x72), city) + 1);
    if (data.stock_dialog.rows > 1) {
      SetRowCheck(ds, event, data.stock_dialog, RowCheck::NONE_3);
      Call(ds, event, ViewService::CONTROL_PANEL);
      Call(ds, event, ViewService::REQUEST_FORMAT, title, CITY_FORMAT, city);
      Call(ds, event, ViewService::CONTROL_MESSAGE, title,
           data.context_record3);
      Call(ds, event, ViewService::COPY_BYTES, LIST_WINDOW, saved_window,
           LIST_WINDOW_STATE);
      Call(ds, event, ViewService::LIST_DIALOG, Near(data, data.stock_dialog),
           1, 7);
      Call(ds, event, ViewService::COPY_BYTES, saved_window, LIST_WINDOW,
           LIST_WINDOW_STATE);
    }
  }
}

// The own side's cities.
void opeu_view_own_cities(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  ViewSideCities(ds, event, sp, Data(ds).current_side, CITY_NONE_OWN);
}

// The enemy side's cities.
void opeu_view_enemy_cities(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto const enemy{ static_cast<std::uint8_t>(
      Call(ds, event, ViewService::OPPOSITE_SIDE, Data(ds).current_side)) };
  ViewSideCities(ds, event, sp, enemy, CITY_NONE_ENEMY);
}

// The cities menu (own, enemy) until cancelled.
void opeu_view_cities_menu(uint8_t ds[65536], HostEvent event, uint16_t) {
  auto& data{ Data(ds) };
  for (std::uint16_t item;
       (item = Call(ds, event, ViewService::DELAY_DIALOG, CITY_SIDE_PROMPT, 2,
                    data.context_record2, CITY_SIDE_MENU)) != MENU_CANCELLED;)
    RunAction(ds, event, data.city_actions, item);
}

uint16_t svc_vwcity(uint8_t* ds, uint16_t id, const uint16_t* args,
                    uint16_t count) {
  return svc_view(ds, "vwcity", id, args, count);
}

auto port_routine_opeu_view_cities_menu(std::uint8_t* ds, std::uint16_t const*,
                                        unsigned) -> std::uint32_t {
  WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
    opeu_view_cities_menu(ds, svc_vwcity, sp);
  });
  return 0;
}

auto port_routine_opeu_view_enemy_cities(std::uint8_t* ds, std::uint16_t const*,
                                         unsigned) -> std::uint32_t {
  WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
    opeu_view_enemy_cities(ds, svc_vwcity, sp);
  });
  return 0;
}

auto port_routine_opeu_view_own_cities(std::uint8_t* ds, std::uint16_t const*,
                                       unsigned) -> std::uint32_t {
  WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
    opeu_view_own_cities(ds, svc_vwcity, sp);
  });
  return 0;
}
