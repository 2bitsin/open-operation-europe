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
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/view-service.hpp"
#include "core/textout.hpp"
#include "core/vwarms.hpp"
#include "link/glue.hpp"
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
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(static_cast<std::uint16_t>(at.x)), static_cast<int16_t>(static_cast<std::uint16_t>(at.y)));
}

// Prints the format with its words at the text cursor.
template <typename... _Words>
void Print(Game& game, std::uint16_t format, _Words... words) {
  opeu_cursor_print(game, std::array<std::uint16_t, 1 + sizeof...(_Words)>{ format, static_cast<std::uint16_t>(words)... });
}

} // namespace

// The objective's name, supplies with their percentages, endurance and the stocks of its garrison.
void opeu_objective_panel(Game& game, uint16_t objective) {
  ServiceLocals const locals{ 0xa }; // the supply percentages the service writes
  auto&               data{ Data(game) };
  auto const&         city{ Record<Objective>(data, objective) };
  auto const          ammunition{ locals.At(8) };
  auto const          food{ locals.At(2) };
  auto const          fuel{ locals.At(0) };
  opeu_army_title(game, static_cast<DialogHost const*>(port_dialogs_host()), objective);
  opeu_objective_resource_percentages(game, objective, port_ds_word_ptr(game, static_cast<std::uint16_t>(ammunition)), port_ds_word_ptr(game, static_cast<std::uint16_t>(fuel)),
                                      port_ds_word_ptr(game, static_cast<std::uint16_t>(food)));
  Position(game, FIRST_ROW);
  Print(game, FUEL_AMMO, city.resources[1], Word(game, fuel), city.resources[0], Word(game, ammunition));
  Position(game, SECOND_ROW);
  Print(game, FOOD_END, city.resources[2], Word(game, food), city.strength, city.strength_limit);
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
  ServiceLocals const locals{ 0x28 }; // the supply percentages, the destination cell and two names services write
  auto&               data{ Data(game) };
  auto const&         regiment{ Record<Unit>(data, unit) };
  auto const          ammunition{ locals.At(0x26) };
  auto const          fuel{ locals.At(0x1c) };
  auto const          food{ locals.At(0x1e) };
  auto const          destination_x{ locals.At(0x1a) };
  auto const          destination_y{ locals.At(0) };
  auto const          location_name{ locals.At(2) };
  opeu_army_title(game, static_cast<DialogHost const*>(port_dialogs_host()), static_cast<std::uint16_t>(opeu_unit_name(game, unit, static_cast<std::uint16_t>(locals.At(0xe)))));
  opeu_unit_capacities(game, unit, port_ds_word_ptr(game, static_cast<std::uint16_t>(ammunition)), port_ds_word_ptr(game, static_cast<std::uint16_t>(fuel)),
                       port_ds_word_ptr(game, static_cast<std::uint16_t>(food)));
  // each capacity becomes the percentage of it the regiment holds
  for (auto const& [kind, local] : {
           std::pair{ 0, ammunition },
           std::pair{ 1, fuel       },
           std::pair{ 2, food       }
  })
    SetWord(game, local, opeu_resource_percent(static_cast<std::uint16_t>(regiment.resources[kind]), static_cast<std::uint16_t>(Word(game, local))));
  auto const has_destination{ opeu_unit_order_destination(game, unit, &Record<std::uint8_t>(Data(game), static_cast<std::uint16_t>(destination_x)),
                                                          &Record<std::uint8_t>(Data(game), static_cast<std::uint16_t>(destination_y))) };
  Position(game, FIRST_ROW);
  auto const order{ has_destination ? data.order_names[regiment.orders[0].kind + ORDER_NAME_BASE] : WAITING };
  Print(game, ORDERS, order, regiment.resources[1], Word(game, fuel));
  Position(game, SECOND_ROW);
  auto const target{ has_destination ? opeu_location_name(game, static_cast<std::uint8_t>(Record<std::uint8_t>(data, destination_x)),
                                                          static_cast<std::uint8_t>(Record<std::uint8_t>(data, destination_y)), static_cast<std::uint16_t>(location_name))
                                     : opeu_location_name(game, static_cast<std::uint8_t>(regiment.x), static_cast<std::uint8_t>(regiment.y), static_cast<std::uint16_t>(location_name)) };
  Print(game, TARGET, target, regiment.resources[0], Word(game, ammunition));
  Position(game, THIRD_ROW);
  Print(game, FATIGUE_ROW, Record<Element>(data, regiment.first).experience, regiment.resources[2], Word(game, food));
  std::uint8_t row{ 0 };
  for (auto element{ regiment.first }; element; element = Record<Element>(data, element).next, row++) {
    auto const& battalion{ Record<Element>(data, element) };
    Position(game, data.element_rows[row]);
    Print(game, ELEMENT_ROW, data.element_types[battalion.type].name, battalion.quantity);
  }
}
