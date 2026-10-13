// Regiment reinforcements of the request menu: loading an element with a random equipment type that suits the
// requested regiment kind (the quantity bonus follows the delivery level), and the player's "regiment reinforcements"
// request.
#include "units/rqreinf.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/rqarms.hpp"

#include <array>
#include <utility>

using namespace opeu::game;

namespace {
// The equipment categories that suit each regiment kind, for each of the two alternatives the caller draws.
// clang-format off
constexpr std::array<std::array<Category, 2>, 6> KIND_CATEGORIES{ {
  { Category::INFANTRY,     Category::INFANTRY     },
  { Category::INFANTRY_X10, Category::INFANTRY_X10 },
  { Category::TANKS,        Category::INFANTRY_X5  },
  { Category::ARTILLERY,    Category::INFANTRY     },
  { Category::ENGINEERS,    Category::ENGINEERS    },
  { Category::SUPPLY,       Category::REPAIR       },
} };
// clang-format on

// The number of equipment types the random draw picks from.
constexpr std::uint16_t TYPE_DRAW{ 158 };

// Whether an equipment type of `category` suits the regiment kind; other kinds take any category.
[[nodiscard]] auto Suits(std::uint8_t kind, std::uint8_t alternative, std::uint8_t category) -> bool {
  if (kind >= KIND_CATEGORIES.size()) return true;
  return category == Bits(KIND_CATEGORIES[kind][alternative]);
}

// The regiment kinds the player chooses from (the last two are fixed by the unit's slot).
constexpr std::uint16_t CHOSEN_KINDS{ 4 };
constexpr std::uint16_t SLOT_6_KIND{ 4 };
constexpr std::uint16_t OTHER_KIND{ 5 };
} // namespace

// Picks a random available equipment type that suits the regiment kind (`side` chooses between the two alternatives
// of a kind) and loads the element with a random quantity of it.
void opeu_reinf_element(Game& game, HostEvent event, uint16_t element, uint8_t kind, uint8_t side) {
  auto& data{ Data(game) };
  auto& loaded{ Record<Element>(data, element) };
  for (;;) {
    auto const index{ static_cast<std::uint8_t>(Call(game, event, RequestService::RANDOM, TYPE_DRAW)) };
    if (!data.type_availability[index]) continue;
    auto const& type{ data.element_types[index] };
    auto const  category{ type.category };
    if (!Suits(kind, side, category)) continue;
    if (!(data.side_masks[data.current_side] & type.sides)) continue;
    if (data.type_availability[index] <= Call(game, event, RequestService::RANDOM, 100)) continue;
    loaded.type           = index;
    std::uint16_t amount  = data.request_capacities[category] / (Call(game, event, RequestService::MAX16, type.anti_tank, type.anti_personnel) / 20 + 1);
    amount               += (5 - data.delivery_level) * 6;
    loaded.quantity       = Call(game, event, RequestService::MIN16, Call(game, event, RequestService::RANDOM, amount / 3) + (amount >> 1), data.request_capacities[category]);
    loaded.damage         = 0;
    return;
  }
}

// "Which regiment should we call in, sir?": cancels a pending regiment request, or files a new one for a free unit
// the player chooses, filling its free elements with fresh equipment.
void opeu_request_regiment(Game& game, HostEvent event) {
  auto&      data{ Data(game) };
  auto const regiment{ std::to_underlying(RequestKind::REGIMENT) };
  auto const requested{ Bits(UnitFlag::REQUESTED) };
  if (Call(game, event, RequestService::PENDING, regiment)) {
    if (!Call(game, event, RequestService::CANCEL_PROMPT, Near(data, data.rq_reinforcements_text))) return;
    Call(game, event, RequestService::FREE_UNITS, 0, requested);
    Call(game, event, RequestService::FREE_ELEMENTS, 0, requested);
    Record<Side>(data, data.current_side_ptr).requests[regiment].kind = std::to_underlying(RequestKind::NONE);
  }
  auto const units{ static_cast<std::uint8_t>(Call(game, event, RequestService::FREE_UNITS, Near(data, data.ai_unit_list), requested)) };
  if (!units) {
    Call(game, event, RequestService::NO_MORE, Near(data, data.rq_regiment_text));
    return;
  }
  auto elements{ static_cast<std::uint8_t>(Call(game, event, RequestService::FREE_ELEMENTS, Near(data, data.ai_element_list), requested)) };
  if (!elements) {
    Call(game, event, RequestService::NO_MORE, Near(data, data.rq_battalion_text));
    return;
  }
  Call(game, event, RequestService::CONTROL_PANEL);
  Call(game, event, RequestService::CONTROL_MESSAGE, Near(data, data.rq_which_regiment_text), data.context_record2);
  data.rq_regiment_list.rows = static_cast<std::uint8_t>(units + 1);
  if (!Call(game, event, RequestService::LIST_DIALOG, Near(data, data.rq_regiment_list), LIST_LIMIT, LIST_FLAGS)) return;
  auto const    unit{ data.ai_unit_list[data.list_row] };
  auto const    slot{ static_cast<std::uint8_t>(Call(game, event, RequestService::UNIT_INDEX, unit)) };
  std::uint16_t kind;
  if (Call(game, event, RequestService::CATEGORY_0_TO_3, slot) || Call(game, event, RequestService::CATEGORY_4_OR_5, slot)) {
    Call(game, event, RequestService::ARMY_TITLE, Near(data, data.rq_regiments_text));
    kind = Call(game, event, RequestService::DELAY_DIALOG, Near(data, data.rq_regiment_kind_text), CHOSEN_KINDS, data.context_record2, Near(data, data.rq_regiment_kind_menu));
    if (kind == 0xffff) return;
    kind--;
  } else if (Call(game, event, RequestService::CATEGORY_6, slot))
    kind = SLOT_6_KIND;
  else
    kind = OTHER_KIND;
  if (!Call(game, event, RequestService::CONFIRM_OK, data.context_record2)) return;
  Record<Unit>(data, unit).flags = requested;
  auto const battalions{ data.ai_unit_element_counts[slot] };
  for (std::uint8_t row{ 0 }; row < battalions; ++row) {
    if (!Call(game, event, RequestService::RANDOM, battalions) && row > 1) break;
    if (!elements) break;
    auto const near{ data.ai_element_list[row] };
    opeu_reinf_element(game, event, near, static_cast<std::uint8_t>(kind), static_cast<std::uint8_t>(Call(game, event, RequestService::RANDOM, 2)));
    auto& element{ Record<Element>(data, near) };
    element.morale     = static_cast<std::uint8_t>(Call(game, event, RequestService::RANDOM, 80));
    element.training   = static_cast<std::uint8_t>(Call(game, event, RequestService::RANDOM, 80));
    element.experience = static_cast<std::uint8_t>(Call(game, event, RequestService::RANDOM, 30));
    element.flags      = Bits(ElementFlag::REQUESTED);
    --elements;
  }
  Call(game, event, RequestService::FILE_REQUEST, 0, static_cast<std::uint8_t>(kind), regiment);
  data.side_delays[data.current_side] = 1;
}

// The services of the regiment request: the request menu's (svc_units_request).
uint16_t svc_rqreinf(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  if (id <= std::to_underlying(RequestService::CONFIRM_OK)) return svc_units_request("rqreinf", game, id, args, count);
  port_unhandled("rqreinf", id, count);
  return 0;
}

auto port_routine_opeu_request_regiment(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_request_regiment(game, svc_rqreinf);
  return 0;
}
