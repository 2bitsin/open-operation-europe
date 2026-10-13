// The information panels the map shows for an objective (supplies, endurance, garrison stocks) and for a regiment
// (orders, target, supplies, fatigue and battalions).
#include "core/vwpanel.hpp"
#include "common/events.hpp"
#include "core/dialogs.hpp"
#include "types.hpp"
#include "units/capacity.hpp"
#include "units/labels.hpp"
#include "units/objpct.hpp"
#include "units/orders.hpp"
#include "units/ratio.hpp"
#include "units/unitname.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/view-service.hpp"
#include "core/textout.hpp"
#include "core/vwarms.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t FUEL_AMMO{ offsetof(DataSegment, panel_fuel_ammo) };
constexpr std::uint16_t FOOD_END{ offsetof(DataSegment, panel_food_end) };
constexpr std::uint16_t GARRISON{ offsetof(DataSegment, panel_garrison) };
constexpr std::uint16_t STOCK_ROW{ offsetof(DataSegment, panel_stock_row) };
constexpr std::uint16_t NO_STOCK{ offsetof(DataSegment, panel_no_stock) };
constexpr std::uint16_t WAITING{ offsetof(DataSegment, panel_waiting) };
constexpr std::uint16_t ORDERS{ offsetof(DataSegment, panel_orders) };
constexpr std::uint16_t TARGET{ offsetof(DataSegment, panel_target) };
constexpr std::uint16_t FATIGUE_ROW{ offsetof(DataSegment, panel_fatigue) };
constexpr std::uint16_t ELEMENT_ROW{ offsetof(DataSegment, panel_element_row) };

// order_names lists three entries before the first order kind.
constexpr unsigned ORDER_NAME_BASE{ 3 };

// Where the panels print their rows, in text columns and pixels.
constexpr ScreenPoint FIRST_ROW{ 0x2d, 0xa3 };
constexpr ScreenPoint SECOND_ROW{ 0x2d, 0xb4 };
constexpr ScreenPoint THIRD_ROW{ 0x2d, 0xc5 };
constexpr ScreenPoint GARRISON_ROW{ 0x32, 0xcc };
constexpr ScreenPoint NO_STOCK_ROW{ 0x2d, 0xdd };

void Position(Game& game, ScreenPoint at) {
  opeu_army_position(game, at.x, at.y);
}

// Prints the format with its words at the text cursor.
template <typename... _Words>
void Print(Game& game, std::uint16_t format, _Words... words) {
  opeu_cursor_print(game, std::array<std::uint16_t, 1 + sizeof...(_Words)>{ format, static_cast<std::uint16_t>(words)... });
}

} // namespace

// The objective's name, supplies with their percentages, endurance and the stocks of its garrison.
void opeu_objective_panel(Game& game, uint16_t objective) {
  auto&       data{ Data(game) };
  auto const& city{ Record<Objective>(data, objective) };
  opeu_army_title(game, objective);
  auto const percent{ ObjectiveResourcePercentages(data, objective) };
  Position(game, FIRST_ROW);
  Print(game, FUEL_AMMO, city.resources[1], percent.fuel, city.resources[0], percent.ammunition);
  Position(game, SECOND_ROW);
  Print(game, FOOD_END, city.resources[2], percent.food, city.strength, city.strength_limit);
  Position(game, GARRISON_ROW);
  Print(game, GARRISON);
  std::uint8_t shown{ 0 };
  for (auto const& item : city.items)
    if (item.quantity) {
      Position(game, data.stock_rows[shown]);
      Print(game, STOCK_ROW, data.element_types[item.type].name, item.quantity);
      shown++;
    }
  if (shown == 0) {
    Position(game, NO_STOCK_ROW);
    Print(game, NO_STOCK);
  }
}

// The regiment's name, its order and target (or "waiting" and its own place), supplies with their percentages, the
// fatigue of its first battalion and its battalions.
void opeu_unit_panel(Game& game, uint16_t unit) {
  ServiceLocals const locals{ 0x28 }; // the two names services write
  auto&               data{ Data(game) };
  auto const&         regiment{ Record<Unit>(data, unit) };
  auto const          location_name{ locals.At(2) };
  opeu_army_title(game, opeu_unit_name(game, unit, locals.At(0xe)));
  auto const capacity{ UnitCapacities(data, unit) };
  auto const ammunition{ opeu_resource_percent(regiment.resources[0], capacity.ammunition) };
  auto const fuel{ opeu_resource_percent(regiment.resources[1], capacity.fuel) };
  auto const food{ opeu_resource_percent(regiment.resources[2], capacity.food) };
  auto const destination{ UnitOrderDestination(game, unit) };
  auto const has_destination{ destination.valid && destination.cell };
  Position(game, FIRST_ROW);
  auto const order{ has_destination ? data.order_names[regiment.orders[0].kind + ORDER_NAME_BASE] : WAITING };
  Print(game, ORDERS, order, regiment.resources[1], fuel);
  Position(game, SECOND_ROW);
  auto const target{ has_destination ? opeu_location_name(game, destination.cell->x, destination.cell->y, location_name) : opeu_location_name(game, regiment.x, regiment.y, location_name) };
  Print(game, TARGET, target, regiment.resources[0], ammunition);
  Position(game, THIRD_ROW);
  Print(game, FATIGUE_ROW, Record<Element>(data, regiment.first).experience, regiment.resources[2], food);
  std::uint8_t row{ 0 };
  for (auto const& battalion : Elements(data, regiment)) {
    Position(game, data.element_rows[row++]);
    Print(game, ELEMENT_ROW, data.element_types[battalion.type].name, battalion.quantity);
  }
}
