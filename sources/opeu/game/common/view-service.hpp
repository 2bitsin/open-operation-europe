// The services the information views (core/vwarms, vwcity, vwdiv, vwpanel, vwpeople, vwregt and units/vwlist) call
// through their event, dispatched by svc_view (core/vwarms), and the helpers those views share. Pointer arguments are
// DS offsets; byte arguments are passed zero-extended.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace opeu::game::detail::view_service {

enum class ViewService : std::uint16_t {
  OBJECTIVE_SIDE = 1,  // (objective) -> side
  OBJECTIVE_ELIGIBLE,  // (objective) -> nonzero when it may be listed
  GROUP_SELECTED_UNIT, // (group) -> unit
  UNIT_ELIGIBLE,       // (unit) -> nonzero when it may be selected
  UNIT_SIDE,           // (unit) -> side
  ARMY_TITLE,          // (text)
  OBJECTIVE_PERCENTS,  // (objective, &ammunition, &fuel, &food)
  ARMY_POSITION,       // (x, y)
  ARMY_PRINT,          // (format, ...)
  UNIT_NAME,           // (unit, buffer) -> text
  UNIT_CAPACITIES,     // (unit, &ammunition, &fuel, &food)
  RESOURCE_PERCENT,    // (current low, high, capacity low, high) -> percent
  ORDER_DESTINATION,   // (unit, &x, &y) -> nonzero when the order has a destination
  LOCATION_NAME,       // (x, y, buffer) -> text
  GROUP_SHOW,          // (group, index)
  BACKDROP_COLOR,      // (color)
  SIDE_RECORD,         // (side, kind) -> person
  GROUP_RECORD,        // (side, group) -> person
  ARMY_HEADING,        // (person)
  DIVISION_HEADING,    // (person)
  DIVISION_UNITS,      // (units, person)
  DIVISION_FINISH,     // ()
  CONTROL_RESET,       // ()
  DAILY_CONTEXT,       // (side)
  CONTROL_POSITION,    // (x, y)
  ARRIVAL_LOCATION,    // (x, y, side)
  CONTROL_MESSAGE,     // (text, person)
  TURN_ACTIVE_BEGIN,   // ()
  DELAY_DIALOG,        // (text, last item, person, menu) -> item, 0xffff when cancelled
  CONTROL_PANEL,       // ()
  LIST_DIALOG,         // (dialog, mode, height) -> nonzero while a row is picked
  OPPOSITE_SIDE,       // (side) -> side
  UNIT_FLASH,          // (record, ticks)
  REQUEST_FORMAT,      // (destination, format, ...)
  COPY_BYTES,          // (source, destination, size)
  UNIT_ANNOUNCE,       // (unit, mode)
  UNIT_SPECIAL,        // (unit) -> nonzero when the enemy regiment is sighted
  TYPE_PANEL,          // (element type)
  FAR_CALL,            // (offset, segment): a menu action from a far pointer table
  CODE_SEGMENT,        // () -> the code segment of the view routines
};

// The row checks of the views' list dialogs (units/vwlist), by their offset in the views' code segment: a list
// dialog offers only the rows its check accepts.
enum class RowCheck : std::uint16_t {
  OBJECTIVE = 0x0120, // own objectives that may be listed, enemy ones that are marked
  NONE      = 0x016e, // no row
  DIVISION  = 0x0172, // a present division whose selected regiment may be selected
  REGIMENT  = 0x01ac, // a present regiment that may be selected if ours, sighted if the enemy's
  NONE_2    = 0x0200, // no row
  NONE_3    = 0x0204, // no row
  ANY       = 0x0208, // every row
};

// What a menu dialog answers when it is cancelled.
inline constexpr std::uint16_t MENU_CANCELLED{ 0xffff };

// The DS stack size of the list window state a view saves around a nested list dialog.
inline constexpr std::uint16_t LIST_WINDOW_STATE{ 7 };

// The entry SP a view passes to another view routine it calls directly, from a routine whose frame holds `frame`
// bytes of locals.
[[nodiscard]] constexpr auto CalleeStack(std::uint16_t sp, std::uint16_t frame) noexcept -> std::uint16_t {
  return static_cast<std::uint16_t>(sp - 6 - frame);
}

// The DS offset of a view's frame local `offset` bytes below its frame pointer (sp - 2).
[[nodiscard]] constexpr auto FrameLocal(std::uint16_t sp, std::uint16_t offset) noexcept -> std::uint16_t {
  return Local(static_cast<std::uint16_t>(sp - 2), offset);
}

// Gives a list dialog its row check.
inline auto SetRowCheck(std::uint8_t* ds, HostEvent event, ListWindow& dialog, RowCheck check) -> void {
  dialog.select.offset  = std::to_underlying(check);
  dialog.select.segment = Call(ds, event, ViewService::CODE_SEGMENT);
}

// Runs the menu action `item` (1 first) of a far pointer table.
template <std::size_t N>
inline auto RunAction(std::uint8_t* ds, HostEvent event, std::array<FarPointer, N> const& actions, std::uint16_t item) -> std::uint16_t {
  auto const& action{ actions[item - 1u] };
  return Call(ds, event, ViewService::FAR_CALL, action.offset, action.segment);
}

} // namespace opeu::game::detail::view_service

namespace opeu::game {
using detail::view_service::CalleeStack;
using detail::view_service::FrameLocal;
using detail::view_service::LIST_WINDOW_STATE;
using detail::view_service::MENU_CANCELLED;
using detail::view_service::RowCheck;
using detail::view_service::RunAction;
using detail::view_service::SetRowCheck;
using detail::view_service::ViewService;
} // namespace opeu::game
