// The Paratroops command: the player picks a regiment and its paratroop battalions in the list dialogs, assigns
// troops to each, picks a drop site and a delay; the air force may delay or refuse the drop, else the order is
// stored as the current side's pending airborne drop.
#include "core/airdrop.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "core/airhelp.hpp"
#include "core/limits.hpp"
#include "core/perssel.hpp"
#include "core/recindex.hpp"
#include "types.hpp"
#include "units/release.hpp"
#include "units/value.hpp"

#include "common/air-service.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/airbomb.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

// The code segment of the list-dialog callbacks this module stores as far pointers (set by the caller).
uint16_t opeu_airdrop_code_segment = 0x23b2;

namespace {

using enum AirService;

// The far offsets of the list-dialog callbacks (the routine handlers below).
constexpr std::uint16_t MARK_REGIMENT_OFFSET{ 0x06ca };
constexpr std::uint16_t ASSIGN_TROOPS_OFFSET{ 0x06de };
constexpr std::uint16_t MARK_BATTALION_OFFSET{ 0x077a };

constexpr std::uint16_t TROOPS_PER_PLANE{ 30 };
constexpr std::uint8_t  NO_TYPE{ 0xff };
constexpr std::uint8_t  CHOSEN{ 0x10 }; // the mask release_side_units/elements clear and count

// Message ids (message_lookup).
constexpr std::uint16_t DROP_REFUSED{ 0x69 };    // the air force refuses a drop by daylight
constexpr std::uint16_t REQUEST_STANDS{ 5 };     // "... %d hours": the requested delay stands
constexpr std::uint16_t AIR_FORCE_DELAY{ 0x72 }; // the air force needs %d hours
constexpr std::uint16_t PLANES_NEEDED{ 0x79 };   // hours, planes, "plane"/"planes"

constexpr std::uint16_t TEXT_BYTES{ 0x66 }; // the message text of drop_delay and drop_order, which the format service writes
// The drop site, which the map pick writes: offsets in the command's reserve.
constexpr std::uint16_t SITE_Y_AT{ 0 };
constexpr std::uint16_t SITE_X_AT{ 2 };
constexpr std::uint16_t SITE_BYTES{ 4 };

auto CurrentDrop(DataSegment& data) -> AirDrop& {
  return Record<Side>(data, data.current_side_ptr).drop;
}

auto ListDialog(std::uint8_t* ds, HostEvent event, ListWindow const& window) -> std::uint16_t {
  return Call(ds, event, LIST_DIALOG, Near(Data(ds), window), 1, 7);
}

auto ReleaseUnits(std::uint8_t* ds, std::uint16_t list) -> std::uint8_t {
  return opeu_release_side_units(ds, list, CHOSEN);
}
auto ReleaseElements(std::uint8_t* ds, std::uint16_t list) -> std::uint8_t {
  return opeu_release_side_elements(ds, list, CHOSEN);
}
} // namespace

// Regiment list callback: marks the highlighted free unit chosen.
uint8_t opeu_air_mark_regiment(uint8_t ds[65536], HostEvent) {
  auto& data{ Data(ds) };
  Record<Unit>(data, data.supply_units[data.list_row]).flags = Bits(UnitFlag::AIRBORNE);
  return 1;
}

// Battalion list callback: asks how many troops the highlighted element takes (bounded by the transport capacity,
// the paratroopers available and the type-0 request capacity) and keeps the side's assigned total.
uint8_t opeu_air_assign_troops(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto&      element{ Record<Element>(data, data.alloc_elements[data.list_row]) };
  auto const most{ opeu_min16(opeu_min16(opeu_air_free_planes(ds, event) * TROOPS_PER_PLANE + element.quantity - data.air_troops, Record<Side>(data, data.current_side_ptr).paratroops),
                              data.request_capacities[0]) };
  auto const troops{ opeu_air_ask_count(ds, event, Near(data, data.drop_ask_troops), 0, most, 1, data.context_record3, 0) };
  if (troops != MENU_CANCELLED) {
    data.air_troops  = static_cast<std::uint16_t>(data.air_troops + (troops - element.quantity));
    element.quantity = troops;
    element.flags    = troops ? Bits(ElementFlag::AIRBORNE) : 0;
  }
  Call(ds, event, MESSAGE, Near(data, data.drop_troops_assigned), data.context_record3);
  return 0;
}

// Command-post list callback: marks the highlighted battalion as the drop's command post.
uint8_t opeu_air_mark_battalion(uint8_t ds[65536], HostEvent) {
  auto& data{ Data(ds) };
  Record<Element>(data, data.alloc_elements[data.list_row]).flags |= Bits(ElementFlag::SELECTED);
  return 1;
}

// The first airborne element type the current side fields, or 0xff.
uint8_t opeu_air_paratroop_type(uint8_t ds[65536], HostEvent) {
  auto const& data{ Data(ds) };
  auto const  mask{ data.side_masks[data.current_side] };
  for (std::uint8_t n{ 0 }; n < ELEMENT_TYPE_COUNT; ++n) {
    auto const& type{ data.element_types[n] };
    if ((type.flags & Bits(TypeFlag::AIRBORNE)) && (type.sides & mask)) return n;
  }
  return NO_TYPE;
}

// Chooses the regiment, its paratroop battalions (each element of the regiment's quota becomes the airborne type with
// no troops until the callback assigns some) and the command post; nonzero when all were chosen, otherwise every
// selection is released.
uint16_t opeu_air_choose_forces(uint8_t ds[65536], HostEvent event) {
  auto&                   data{ Data(ds) };
  constexpr std::uint16_t SUPPLY_UNITS{ offsetof(DataSegment, supply_units) };
  constexpr std::uint16_t ALLOC_ELEMENTS{ offsetof(DataSegment, alloc_elements) };
  data.regiment_dialog.rows = static_cast<std::uint8_t>(ReleaseUnits(ds, SUPPLY_UNITS) + 1);
  SetListPredicate(data.regiment_dialog, MARK_REGIMENT_OFFSET, opeu_airdrop_code_segment);
  Call(ds, event, MESSAGE, Near(data, data.drop_which_regiment), data.context_record3);
  if (ListDialog(ds, event, data.regiment_dialog)) {
    // The regiment's battalion quota is read again on every step, as the dialogs run in between.
    auto const regiment{ opeu_unit_index(data.supply_units[data.list_row]) };
    auto const quota{ [&] { return data.ai_unit_element_counts[regiment]; } };
    auto const free{ static_cast<std::uint8_t>(opeu_min16(ReleaseElements(ds, ALLOC_ELEMENTS), quota())) };
    data.choice_dialog.rows = static_cast<std::uint8_t>(free + 1);
    SetListPredicate(data.choice_dialog, ASSIGN_TROOPS_OFFSET, opeu_airdrop_code_segment);
    auto const type{ opeu_air_paratroop_type(ds, event) };
    for (std::uint8_t i{ 0 }; i < quota() && free > i; ++i) {
      auto& element{ Record<Element>(data, data.alloc_elements[i]) };
      element.type     = type;
      element.quantity = 0;
      element.damage   = 0;
    }
    data.air_troops = 0;
    Call(ds, event, MESSAGE, Near(data, data.drop_assign_soldiers), data.context_record3);
    ListDialog(ds, event, data.choice_dialog);
    std::uint8_t kept{ 0 };
    for (std::uint8_t i{ 0 }; i < quota() && free > i; ++i) {
      auto const element{ data.alloc_elements[i] };
      if (Record<Element>(data, element).quantity) data.alloc_elements[kept++] = element;
    }
    if (kept) {
      data.choice_dialog.rows = static_cast<std::uint8_t>(kept + 1);
      SetListPredicate(data.choice_dialog, MARK_BATTALION_OFFSET, opeu_airdrop_code_segment);
      Call(ds, event, MESSAGE, Near(data, data.drop_command_post), data.context_record3);
      if (ListDialog(ds, event, data.choice_dialog)) return 1;
    }
  }
  ReleaseUnits(ds, 0);
  ReleaseElements(ds, 0);
  return 0;
}

// Hours until the drop. The air force's own estimate (6..24 from the troops, the delivery level and its staff's
// ratings) beats a shorter requested delay: it is announced, or on a rating roll the request stands. A requested
// landing by daylight may be refused outright (0 returned).
uint8_t opeu_air_drop_delay(uint8_t ds[65536], HostEvent event, uint8_t hours) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(ds) };
  auto const          text{ locals.At(0) };
  std::uint8_t const  estimate{ static_cast<std::uint8_t>(opeu_min16(opeu_max16(RandomBelow(ds, data.delivery_level + 2) - data.air_leadership / 30 + data.air_troops / 300 + 6, 6), 24)) };
  if (estimate > hours) {
    if (95 - static_cast<std::uint8_t>(RandomBelow(ds, 20)) > data.air_charm) {
      Call(ds, event, FORMAT, text, Call(ds, event, LOOKUP, AIR_FORCE_DELAY), estimate);
      Call(ds, event, MESSAGE, text, data.air_person);
      return estimate;
    }
    Call(ds, event, FORMAT, text, Call(ds, event, LOOKUP, REQUEST_STANDS), estimate + 1);
    Call(ds, event, MESSAGE, text, data.air_person);
    return hours;
  }
  if (opeu_hour_is_day(static_cast<std::uint8_t>((hours + data.hour) % 24)) && 60 - static_cast<std::uint8_t>(RandomBelow(ds, 20)) > data.air_charm) {
    Call(ds, event, MESSAGE, Call(ds, event, LOOKUP, DROP_REFUSED), data.air_person);
    return 0;
  }
  Call(ds, event, MESSAGE, Near(data, data.drop_understood), data.air_person);
  return hours;
}

// Reports the planes needed (30 troops each), asks the air force to confirm and records the drop order.
uint16_t opeu_air_drop_order(uint8_t ds[65536], HostEvent event, uint8_t x, uint8_t y, uint8_t hours) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(ds) };
  auto const          text{ locals.At(0) };
  auto const          troops{ data.air_troops };
  auto const          planes{ static_cast<std::uint16_t>((troops % TROOPS_PER_PLANE > 0) + troops / TROOPS_PER_PLANE) };
  Call(ds, event, FORMAT, text, Call(ds, event, LOOKUP, PLANES_NEEDED), hours, planes, planes == 1 ? Near(data, data.drop_plane) : Near(data, data.drop_planes));
  Call(ds, event, MESSAGE, text, data.air_person);
  if (opeu_air_confirm(ds, event)) {
    auto& drop{ CurrentDrop(data) };
    drop.x          = x;
    drop.y          = y;
    drop.paratroops = data.air_troops;
    drop.transports = planes;
    drop.hours      = hours;
    drop.due        = static_cast<std::uint8_t>(hours + 6);
    return 1;
  }
  return 0;
}

// The Paratroops command: cancels a pending drop if asked, checks paratroopers, transports, free regiments and
// battalions, chooses the forces and a valid drop site, asks the delay (6..24 hours) and confirms with Air Force HQ;
// the side's turn ends once the drop is refused (delay 0) or ordered.
void opeu_air_drop_command(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto const message{ [&](auto const& text) { Call(ds, event, MESSAGE, Near(data, text), data.context_record3); } };
  auto const confirm{ [&](auto const& text) { return Call(ds, event, CONFIRM, Near(data, text), data.context_record3); } };
  if (opeu_air_paratroop_type(ds, event) == NO_TYPE || Record<Side>(data, data.current_side_ptr).paratroops == 0) {
    message(data.drop_no_paratroopers);
    return;
  }
  if (CurrentDrop(data).transports) {
    if (!confirm(data.drop_cancel)) return;
    ReleaseUnits(ds, 0);
    ReleaseElements(ds, 0);
    CurrentDrop(data).transports = 0;
  }
  if (!opeu_air_free_planes(ds, event)) {
    message(data.drop_no_transports);
    return;
  }
  if (!ReleaseUnits(ds, 0)) {
    message(data.drop_no_regiments);
    return;
  }
  if (!ReleaseElements(ds, 0)) {
    message(data.drop_no_battalions);
    return;
  }
  if (!opeu_air_choose_forces(ds, event)) return;
  ServiceLocals const locals{ SITE_BYTES };
  auto const          site_x{ locals.At(SITE_X_AT) };
  auto const          site_y{ locals.At(SITE_Y_AT) };
  auto&               x{ Record<std::uint8_t>(data, site_x) };
  auto&               y{ Record<std::uint8_t>(data, site_y) };
  x = static_cast<std::uint8_t>(data.viewport_x);
  y = static_cast<std::uint8_t>(data.viewport_y);
  do {
    message(data.drop_where);
    if (!Call(ds, event, PICK, site_x, site_y)) return;
  } while (!opeu_air_target_valid(ds, event, x, y, Near(data, data.drop_paratroopers)));
  auto const hours{ opeu_air_ask_count(ds, event, Near(data, data.drop_hours_prompt), 6, 24, 1, data.context_record3, 0) };
  if (hours == MENU_CANCELLED) return;
  if (!confirm(data.drop_ask_hq)) return;
  auto const delay{ opeu_air_drop_delay(ds, event, static_cast<std::uint8_t>(hours)) };
  if (delay == 0 || opeu_air_drop_order(ds, event, x, y, delay)) data.side_delays[data.current_side] = 1;
}

uint16_t svc_airdrop(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_air(ds, "airdrop", id, args, count);
}

auto port_routine_opeu_air_assign_troops(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_air_assign_troops(ds, svc_airdrop);
}

auto port_routine_opeu_air_mark_battalion(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_air_mark_battalion(ds, svc_airdrop);
}

auto port_routine_opeu_air_mark_regiment(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_air_mark_regiment(ds, svc_airdrop);
}

auto port_routine_opeu_air_drop_command(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_air_drop_command(ds, svc_airdrop);
  return 0;
}
