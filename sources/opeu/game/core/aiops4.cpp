// The computer side's requests: the side's request slots take a short objective's supplies (slot 0) or an equipment
// type and the objective it goes to (slot 1).
#include "types.hpp"
#include "core/aiops4.hpp"

#include "common/data-segment.hpp"
#include "core/aiops1.hpp"
#include "core/random.hpp"
#include "map/query.hpp"

#include <array>

using namespace opeu::game;

namespace {

constexpr std::uint8_t  ENTRIES_PER_SIDE{ 21 };
constexpr std::uint8_t  FREE_SLOT{ 0xff };
constexpr std::uint8_t  NO_CATEGORY{ 0xff };
constexpr std::uint8_t  NO_ITEM{ 0xff };
constexpr std::uint16_t SHORT_STOCK{
  50000
}; // an objective with less of a supply asks for more
constexpr std::uint8_t RESOURCE_REQUEST{
  0
}; // RequestSlot::kind of a supply request (slot 0)
constexpr std::uint8_t EQUIPMENT_REQUEST{
  1
}; // and of an equipment request (slot 1)
// The three support categories whose stock the side tops up first, in the order of their totals below.
constexpr std::array<std::uint8_t, 3> SUPPORT_CATEGORIES{ 5, 6, 7 };
// Where the equipment-type service lists the types of a category (inside the DS stack, above the frames in use).
constexpr std::uint16_t TYPE_SCRATCH{ 0xfe00 };

using TypeList = std::array<std::uint8_t, ELEMENT_TYPE_COUNT>;

auto CurrentSide(DataSegment& data) -> Side& {
  return Record<Side>(data, data.current_side_ptr);
}

auto IsSupport(std::uint8_t category) -> bool {
  return category == 5 || category == 6 || category == 7;
}

// The element types of `category` the nations in `mask` field, listed by the service; returns their count.
auto AvailableTypes(std::uint8_t* ds, std::uint8_t mask, std::uint8_t category)
    -> std::uint8_t {
  std::uint16_t list{ TYPE_SCRATCH };
  return static_cast<std::uint8_t>(
      AiCall(ds, AiService::DUE_RECORDS, mask, category, &list));
}

// A random one of the listed types (the list holds `count` of them).
auto RandomType(std::uint8_t* ds, std::uint8_t count) -> std::uint8_t {
  return Record<TypeList>(Data(ds), TYPE_SCRATCH)[opeu_rand_bounded(count)];
}
} // namespace

// Whether the side's request slot is taken.
uint16_t opeu_ai_request_pending(uint8_t ds[65536], uint8_t slot) {
  return CurrentSide(Data(ds)).requests[slot].kind != FREE_SLOT ? 1 : 0;
}

// Whether the side's request configuration has this kind.
uint16_t opeu_ai_config_kind_is(uint8_t ds[65536], uint8_t kind) {
  auto const& data{ Data(ds) };
  return data.request_configs[data.current_side].kind == kind ? 1 : 0;
}

// A random objective of the side that is short (below 50000) of any supply, or 0.
uint16_t opeu_ai_short_objective(uint8_t ds[65536]) {
  auto&         data{ Data(ds) };
  std::uint16_t short_of[OBJECTIVE_COUNT];
  std::uint8_t  count{ 0 };
  for (auto& objective : data.objectives) {
    auto const near{ Near(data, objective) };
    if (objective.active &&
        opeu_objective_side(ds, near) == data.current_side &&
        (objective.resources[0] < SHORT_STOCK ||
         objective.resources[1] < SHORT_STOCK ||
         objective.resources[2] < SHORT_STOCK))
      short_of[count++] = near;
  }
  return count ? short_of[opeu_rand_bounded(count)] : 0;
}

// Files a supply request for a short objective in slot 0, unless the slot is taken or the side's configuration is
// of the supply kind. 1 when filed.
uint16_t opeu_ai_request_resources(uint8_t ds[65536]) {
  if (opeu_ai_request_pending(ds, 0) ||
      opeu_ai_config_kind_is(ds, RESOURCE_REQUEST))
    return 0;
  auto const objective{ opeu_ai_short_objective(ds) };
  if (!objective)
    return 0;
  auto& request{ CurrentSide(Data(ds)).requests[0] };
  request.target   = objective;
  request.argument = NO_ITEM;
  request.kind     = RESOURCE_REQUEST;
  return 1;
}

// One of the side's linked objectives with an empty item slot or one holding `type`, drawn with a weight that rises
// with the objective's level (and falls with it for the support categories); 0 when none is drawn.
uint16_t opeu_ai_destination_for_type(uint8_t ds[65536], uint8_t type) {
  auto&         data{ Data(ds) };
  std::uint16_t room[ENTRIES_PER_SIDE];
  std::uint8_t  count{ 0 };
  for (std::uint8_t i{ 0 }; i < ENTRIES_PER_SIDE; ++i) {
    auto const near{
      data.objective_links[data.current_side * ENTRIES_PER_SIDE + i]
    };
    if (!near || !Record<Objective>(data, near).active ||
        opeu_objective_side(ds, near) != data.current_side)
      continue;
    for (auto const& item : Record<Objective>(data, near).items)
      if (!item.quantity || item.type == type) {
        room[count++] = near;
        break;
      }
  }
  auto const    category{ data.element_types[type].category };
  std::uint16_t destination{ 0 };
  for (std::uint8_t i{ 0 }; i < count; ++i) {
    auto weight{ static_cast<std::uint8_t>(
        (static_cast<std::uint8_t>(AiObjectiveLevel(ds, room[i])) + 1) * 18) };
    if (IsSupport(category))
      weight = static_cast<std::uint8_t>(100 - weight);
    if (opeu_rand_bounded(100) < weight)
      destination = room[i];
  }
  return destination;
}

// The support category (7, then 5, then 6) whose average stock over the side's ready elements of it (counting the
// objectives' items too) is below its request capacity, or 255.
uint8_t opeu_ai_short_category(uint8_t ds[65536]) {
  auto&         data{ Data(ds) };
  std::uint8_t  elements[3]{};
  std::uint16_t totals[3]{};
  auto const    support_index{ [](std::uint8_t category) {
    return category - SUPPORT_CATEGORIES[0];
  } };
  for (unsigned i{ 0 }; i < ELEMENTS_PER_SIDE; ++i) {
    auto const& element{
      data.elements[data.current_side * ELEMENTS_PER_SIDE + i]
    };
    if (!(element.flags & Bits(ElementFlag::READY)))
      continue;
    auto const category{ data.element_types[element.type].category };
    if (IsSupport(category)) {
      ++elements[support_index(category)];
      totals[support_index(category)] += element.quantity;
    }
  }
  for (auto const& objective : data.objectives) {
    if (!objective.active)
      continue;
    for (auto const& item : objective.items) {
      if (!item.quantity)
        continue;
      auto const category{ data.element_types[item.type].category };
      if (IsSupport(category))
        totals[support_index(category)] += item.quantity;
    }
  }
  for (auto const category :
       { std::uint8_t{ 7 }, std::uint8_t{ 5 }, std::uint8_t{ 6 } }) {
    auto const i{ support_index(category) };
    if (elements[i] &&
        totals[i] / elements[i] < data.request_capacities[category])
      return category;
  }
  return NO_CATEGORY;
}

// One of the five categories the computer side requests, at random.
uint8_t opeu_ai_random_category(uint8_t ds[65536]) {
  return Data(ds).ai_categories[opeu_rand_bounded(5)];
}

// Files an equipment request in slot 1, unless the slot is taken or the side's configuration is of the equipment
// kind: a random type of a short support category (one time in three) or of a random category, sent to an objective
// with room for it. 1 when filed.
uint16_t opeu_ai_request_equipment(uint8_t ds[65536]) {
  if (opeu_ai_request_pending(ds, 1) ||
      opeu_ai_config_kind_is(ds, EQUIPMENT_REQUEST))
    return 0;
  auto&        data{ Data(ds) };
  std::uint8_t category{ NO_CATEGORY };
  if (!opeu_rand_bounded(3))
    category = opeu_ai_short_category(ds);
  if (category == NO_CATEGORY)
    category = opeu_ai_random_category(ds);
  auto const type{ RandomType(
      ds, AvailableTypes(ds, data.side_masks[data.current_side], category)) };
  auto const destination{ opeu_ai_destination_for_type(ds, type) };
  if (!destination)
    return 0;
  auto& request{ CurrentSide(data).requests[1] };
  request.target   = destination;
  request.argument = type;
  request.kind     = EQUIPMENT_REQUEST;
  return 1;
}
