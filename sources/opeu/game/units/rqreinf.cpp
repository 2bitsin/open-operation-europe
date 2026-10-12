// Regiment reinforcements of the request menu: loading an element with a random equipment type that suits the
// requested regiment kind (the quantity bonus follows the delivery level), and the player's "regiment reinforcements"
// request.
#include "types.hpp"
#include "units/rqreinf.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "units/rqarms.hpp"

#include <array>
#include <utility>

using namespace opeu::game;

namespace {
// The equipment categories that suit each regiment kind, for each of the two alternatives the caller draws.
constexpr std::array<std::array<std::uint8_t, 2>, 6> KIND_CATEGORIES{ {
    { 0, 0 },
    { 1, 1 },
    { 3, 2 },
    { 4, 0 },
    { 5, 5 },
    { 7, 6 },
} };

// The number of equipment types the random draw picks from.
constexpr std::uint16_t TYPE_DRAW{ 158 };

// Whether an equipment type of `category` suits the regiment kind; other kinds take any category.
[[nodiscard]] auto Suits(std::uint8_t kind, std::uint8_t alternative,
                         std::uint8_t category) -> bool {
  if (kind >= KIND_CATEGORIES.size())
    return true;
  return category == KIND_CATEGORIES[kind][alternative];
}

// The regiment kinds the player chooses from (the last two are fixed by the unit's slot).
constexpr std::uint16_t CHOSEN_KINDS{ 4 };
constexpr std::uint16_t SLOT_6_KIND{ 4 };
constexpr std::uint16_t OTHER_KIND{ 5 };
} // namespace

// Picks a random available equipment type that suits the regiment kind (`side` chooses between the two alternatives
// of a kind) and loads the element with a random quantity of it.
void opeu_reinf_element(uint8_t ds[65536], HostEvent event, uint16_t element,
                        uint8_t kind, uint8_t side) {
  auto& data{ Data(ds) };
  auto& loaded{ Record<Element>(data, element) };
  for (;;) {
    auto const index{ static_cast<std::uint8_t>(
        Call(ds, event, RequestService::RANDOM, TYPE_DRAW)) };
    if (!data.request_weights1[index])
      continue;
    auto const& type{ data.element_types[index] };
    auto const  category{ type.category };
    if (!Suits(kind, side, category))
      continue;
    if (!(data.side_masks[data.current_side] & type.sides))
      continue;
    if (data.request_weights1[index] <=
        Call(ds, event, RequestService::RANDOM, 100))
      continue;
    loaded.type          = index;
    std::uint16_t amount = data.request_capacities[category] /
                           (Call(ds, event, RequestService::MAX16,
                                 type.soft_rating, type.hard_rating) /
                                20 +
                            1);
    amount += (5 - data.delivery_level) * 6;
    loaded.quantity = Call(ds, event, RequestService::MIN16,
                           Call(ds, event, RequestService::RANDOM, amount / 3) +
                               (amount >> 1),
                           data.request_capacities[category]);
    loaded.damage   = 0;
    return;
  }
}

// "Which regiment should we call in, sir?": cancels a pending regiment request, or files a new one for a free unit
// the player chooses, filling its free elements with fresh equipment.
void opeu_request_regiment(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto const regiment{ std::to_underlying(RequestKind::REGIMENT) };
  auto const requested{ Bits(UnitFlag::REQUESTED) };
  if (Call(ds, event, RequestService::PENDING, regiment)) {
    if (!Call(ds, event, RequestService::CANCEL_PROMPT,
              Near(data, data.rq_reinforcements_text)))
      return;
    Call(ds, event, RequestService::FREE_UNITS, 0, requested);
    Call(ds, event, RequestService::FREE_ELEMENTS, 0, requested);
    Record<Side>(data, data.current_side_ptr).requests[regiment].kind =
        std::to_underlying(RequestKind::NONE);
  }
  auto const units{ static_cast<std::uint8_t>(
      Call(ds, event, RequestService::FREE_UNITS, Near(data, data.ai_unit_list),
           requested)) };
  if (!units) {
    Call(ds, event, RequestService::NO_MORE, Near(data, data.rq_regiment_text));
    return;
  }
  auto elements{ static_cast<std::uint8_t>(
      Call(ds, event, RequestService::FREE_ELEMENTS,
           Near(data, data.ai_element_list), requested)) };
  if (!elements) {
    Call(ds, event, RequestService::NO_MORE,
         Near(data, data.rq_battalion_text));
    return;
  }
  Call(ds, event, RequestService::CONTROL_PANEL);
  Call(ds, event, RequestService::CONTROL_MESSAGE,
       Near(data, data.rq_which_regiment_text), data.context_record2);
  data.rq_regiment_list.rows = static_cast<std::uint8_t>(units + 1);
  if (!Call(ds, event, RequestService::LIST_DIALOG,
            Near(data, data.rq_regiment_list), 1, 7))
    return;
  auto const    unit{ data.ai_unit_list[data.list_row] };
  auto const    slot{ static_cast<std::uint8_t>(
      Call(ds, event, RequestService::UNIT_INDEX, unit)) };
  std::uint16_t kind;
  if (Call(ds, event, RequestService::CATEGORY_0_TO_3, slot) ||
      Call(ds, event, RequestService::CATEGORY_4_OR_5, slot)) {
    Call(ds, event, RequestService::ARMY_TITLE,
         Near(data, data.rq_regiments_text));
    kind = Call(ds, event, RequestService::DELAY_DIALOG,
                Near(data, data.rq_regiment_kind_text), CHOSEN_KINDS,
                data.context_record2, Near(data, data.rq_slash_text));
    if (kind == 0xffff)
      return;
    kind--;
  } else if (Call(ds, event, RequestService::CATEGORY_6, slot))
    kind = SLOT_6_KIND;
  else
    kind = OTHER_KIND;
  if (!Call(ds, event, RequestService::CONFIRM_OK, data.context_record2))
    return;
  Record<Unit>(data, unit).flags = requested;
  auto const battalions{ data.ai_unit_element_counts[slot] };
  for (std::uint8_t row{ 0 }; row < battalions; ++row) {
    if (!Call(ds, event, RequestService::RANDOM, battalions) && row > 1)
      break;
    if (!elements)
      break;
    auto const near{ data.ai_element_list[row] };
    opeu_reinf_element(
        ds, event, near, static_cast<std::uint8_t>(kind),
        static_cast<std::uint8_t>(Call(ds, event, RequestService::RANDOM, 2)));
    auto& element{ Record<Element>(data, near) };
    element.morale =
        static_cast<std::uint8_t>(Call(ds, event, RequestService::RANDOM, 80));
    element.training =
        static_cast<std::uint8_t>(Call(ds, event, RequestService::RANDOM, 80));
    element.experience =
        static_cast<std::uint8_t>(Call(ds, event, RequestService::RANDOM, 30));
    element.flags = Bits(ElementFlag::REQUESTED);
    --elements;
  }
  Call(ds, event, RequestService::FILE_REQUEST, 0,
       static_cast<std::uint8_t>(kind), regiment);
  data.side_delays[data.current_side] = 1;
}

// The services of the regiment request: the request menu's (svc_units_request).
uint16_t svc_rqreinf(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  if (id <= std::to_underlying(RequestService::CONFIRM_OK))
    return svc_units_request("rqreinf", ds, id, args, count);
  port_unhandled("rqreinf", id, count);
  return 0;
}

auto port_routine_opeu_request_regiment(std::uint8_t*        ds,
                                        std::uint16_t const* args,
                                        unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_request_regiment(ds, svc_rqreinf);
  return 0;
}
