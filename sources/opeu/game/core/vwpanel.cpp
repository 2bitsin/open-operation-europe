// The information panels the map shows for an objective (supplies, endurance, garrison stocks) and for a regiment
// (orders, target, supplies, fatigue and battalions).
#include "types.hpp"
#include "core/vwpanel.hpp"
#include "types/events.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/view-service.hpp"
#include "core/vwarms.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

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

// Frame locals whose addresses the services receive: the three supply percentages, the order's destination cell and
// two name buffers.
constexpr std::uint16_t AMMUNITION_LOCAL{ 2 };
constexpr std::uint16_t OBJECTIVE_FOOD_LOCAL{ 8 };
constexpr std::uint16_t OBJECTIVE_FUEL_LOCAL{ 0xa };
constexpr std::uint16_t UNIT_FOOD_LOCAL{ 0xa };
constexpr std::uint16_t UNIT_FUEL_LOCAL{ 0xc };
constexpr std::uint16_t DESTINATION_X_LOCAL{ 0xe };
constexpr std::uint16_t UNIT_NAME_LOCAL{ 0x1a };
constexpr std::uint16_t LOCATION_NAME_LOCAL{ 0x26 };
constexpr std::uint16_t DESTINATION_Y_LOCAL{ 0x28 };

void Position(std::uint8_t* ds, HostEvent event, ScreenPoint at) {
  Call(ds, event, ViewService::ARMY_POSITION, at.x, at.y);
}

template <typename... Words>
void Print(std::uint8_t* ds, HostEvent event, std::uint16_t format,
           Words... words) {
  Call(ds, event, ViewService::ARMY_PRINT, format, words...);
}

} // namespace

// The objective's name, supplies with their percentages, endurance and the stocks of its garrison.
void opeu_objective_panel(uint8_t ds[65536], HostEvent event, uint16_t sp,
                          uint16_t objective) {
  auto&       data{ Data(ds) };
  auto const& city{ Record<Objective>(data, objective) };
  auto const  ammunition{ FrameLocal(sp, AMMUNITION_LOCAL) };
  auto const  food{ FrameLocal(sp, OBJECTIVE_FOOD_LOCAL) };
  auto const  fuel{ FrameLocal(sp, OBJECTIVE_FUEL_LOCAL) };
  Call(ds, event, ViewService::ARMY_TITLE, objective);
  Call(ds, event, ViewService::OBJECTIVE_PERCENTS, objective, ammunition, fuel,
       food);
  Position(ds, event, FIRST_ROW);
  Print(ds, event, FUEL_AMMO, city.resources[1], Word(ds, fuel),
        city.resources[0], Word(ds, ammunition));
  Position(ds, event, SECOND_ROW);
  Print(ds, event, FOOD_END, city.resources[2], Word(ds, food), city.strength,
        city.strength_limit);
  Position(ds, event, GARRISON_ROW);
  Print(ds, event, GARRISON);
  std::uint8_t shown{ 0 };
  for (auto const& item : city.items)
    if (item.quantity) {
      Position(ds, event, data.stock_rows[shown]);
      Print(ds, event, STOCK_ROW, data.element_types[item.type].name,
            item.quantity);
      shown++;
    }
  if (shown == 0) {
    Position(ds, event, NO_STOCK_ROW);
    Print(ds, event, NO_STOCK);
  }
}

// The regiment's name, its order and target (or "waiting" and its own place), supplies with their percentages, the
// fatigue of its first battalion and its battalions.
void opeu_unit_panel(uint8_t ds[65536], HostEvent event, uint16_t sp,
                     uint16_t unit) {
  auto&       data{ Data(ds) };
  auto const& regiment{ Record<Unit>(data, unit) };
  auto const  ammunition{ FrameLocal(sp, AMMUNITION_LOCAL) };
  auto const  fuel{ FrameLocal(sp, UNIT_FUEL_LOCAL) };
  auto const  food{ FrameLocal(sp, UNIT_FOOD_LOCAL) };
  auto const  destination_x{ FrameLocal(sp, DESTINATION_X_LOCAL) };
  auto const  destination_y{ FrameLocal(sp, DESTINATION_Y_LOCAL) };
  auto const  location_name{ FrameLocal(sp, LOCATION_NAME_LOCAL) };
  Call(ds, event, ViewService::ARMY_TITLE,
       Call(ds, event, ViewService::UNIT_NAME, unit,
            FrameLocal(sp, UNIT_NAME_LOCAL)));
  Call(ds, event, ViewService::UNIT_CAPACITIES, unit, ammunition, fuel, food);
  // each capacity becomes the percentage of it the regiment holds
  for (auto const [kind, local] :
       { std::pair{ 0, ammunition }, std::pair{ 1, fuel },
         std::pair{ 2, food } })
    SetWord(ds, local,
            Call(ds, event, ViewService::RESOURCE_PERCENT,
                 regiment.resources[kind], 0, Word(ds, local), 0));
  auto const has_destination{ Call(ds, event, ViewService::ORDER_DESTINATION,
                                   unit, destination_x, destination_y) };
  Position(ds, event, FIRST_ROW);
  auto const order{
    has_destination
        ? data.order_names[regiment.orders[0].kind + ORDER_NAME_BASE]
        : WAITING
  };
  Print(ds, event, ORDERS, order, regiment.resources[1], Word(ds, fuel));
  Position(ds, event, SECOND_ROW);
  auto const target{ has_destination
                         ? Call(ds, event, ViewService::LOCATION_NAME,
                                Record<std::uint8_t>(data, destination_x),
                                Record<std::uint8_t>(data, destination_y),
                                location_name)
                         : Call(ds, event, ViewService::LOCATION_NAME,
                                regiment.x, regiment.y, location_name) };
  Print(ds, event, TARGET, target, regiment.resources[0], Word(ds, ammunition));
  Position(ds, event, THIRD_ROW);
  Print(ds, event, FATIGUE_ROW,
        Record<Element>(data, regiment.first).experience, regiment.resources[2],
        Word(ds, food));
  std::uint8_t row{ 0 };
  for (auto element{ regiment.first }; element;
       element = Record<Element>(data, element).next, row++) {
    auto const& battalion{ Record<Element>(data, element) };
    Position(ds, event, data.element_rows[row]);
    Print(ds, event, ELEMENT_ROW, data.element_types[battalion.type].name,
          battalion.quantity);
  }
}

uint16_t svc_vwpanel(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  return svc_view(ds, "vwpanel", id, args, count);
}
