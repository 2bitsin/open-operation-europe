// The cities views: lists of the own and the enemy side's cities, each city's stocks, and the menu that picks a side.
#include "core/vwcity.hpp"
#include "common/events.hpp"
#include "core/dialogs.hpp"
#include "core/memory.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "io/listdlg.hpp"
#include "map/mapunit.hpp"
#include "types.hpp"
#include "units/vwlist.hpp"

#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
#include "common/game.hpp"
#include "common/view-service.hpp"
#include "core/vwarms.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t CITY_PROMPT{ offsetof(DataSegment, city_prompt) };
constexpr std::uint16_t CITY_FORMAT{ offsetof(DataSegment, city_format) };
constexpr std::uint16_t CITY_NONE_OWN{ offsetof(DataSegment, city_none_own) };
constexpr std::uint16_t CITY_NONE_ENEMY{ offsetof(DataSegment, city_none_enemy) };
constexpr std::uint16_t CITY_SIDE_PROMPT{ offsetof(DataSegment, city_side_prompt) };
constexpr std::uint16_t CITY_SIDE_MENU{ offsetof(DataSegment, city_side_menu) };
constexpr std::uint16_t TITLE_BYTES{ 0x64 }; // the stock list's title

// Lists the side's cities and shows them; tells when the side holds none.
void ViewSideCities(Game& game, std::uint8_t side, std::uint16_t none) {
  auto&      data{ Data(game) };
  auto const count{ opeu_list_side_objectives(game, side) };
  if (count == 0) {
    opeu_control_message(game, none, data.context_record3);
    return;
  }
  opeu_view_city_list(game, count);
}

// The cities view's sides, by the enumerator city_actions holds for the choice.
constexpr auto CITY_ACTIONS = DispatchRows<CityAction, auto (*)(Game&)->void>({
    { CityAction::OWN,   opeu_view_own_cities   },
    { CityAction::ENEMY, opeu_view_enemy_cities },
});
} // namespace

// Picks a listed city and flashes it; a city that holds stocks has them listed, the city list's window state kept
// aside meanwhile.
void opeu_view_city_list(Game& game, uint8_t count) {
  TextBuffer<TITLE_BYTES> title;
  auto&                   data{ Data(game) };
  data.city_dialog.rows = static_cast<uint8_t>(count + 1);
  SetListPredicate(data.city_dialog, ListSelect::VIEW_OBJECTIVE);
  for (std::uint16_t height{ 7 };
       opeu_title_panel_begin(game), opeu_control_message(game, CITY_PROMPT, data.context_record3), opeu_list_dialog(game, data.city_dialog, { 1, static_cast<std::uint8_t>(height) }); height = 6) {
    auto const city{ data.view_objectives[data.list_row] };
    opeu_unit_flash(game, city, 4);
    data.stock_dialog.rows = static_cast<uint8_t>(opeu_list_objective_stocks(game, city) + 1);
    if (data.stock_dialog.rows > 1) {
      SetListPredicate(data.stock_dialog, ListSelect::VIEW_NONE_3);
      opeu_title_panel_begin(game);
      opeu_request_format(game, title, std::array<FormatArgument, 2>{ CITY_FORMAT, city });
      opeu_control_message(game, title, data.context_record3);
      auto const saved_window{ SaveListWindow(game) };
      opeu_list_dialog(game, data.stock_dialog, LIST_OPTIONS);
      RestoreListWindow(game, saved_window);
    }
  }
}

// The own side's cities.
void opeu_view_own_cities(Game& game) {
  ViewSideCities(game, Data(game).current_side, CITY_NONE_OWN);
}

// The enemy side's cities.
void opeu_view_enemy_cities(Game& game) {
  auto const enemy{ (opeu_opposite_side(Data(game).current_side)) };
  ViewSideCities(game, enemy, CITY_NONE_ENEMY);
}

// The cities menu (own, enemy) until cancelled.
void opeu_view_cities_menu(Game& game) {
  auto& data{ Data(game) };
  for (std::uint16_t item; (item = opeu_delay_dialog(game, CITY_SIDE_PROMPT, 2, data.context_record2, CITY_SIDE_MENU)) != MENU_CANCELLED;)
    Dispatch(CITY_ACTIONS, data.city_actions.at(item - 1u), game);
}
