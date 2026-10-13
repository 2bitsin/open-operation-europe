// Regiment reinforcements of the request menu: loading an element with a random equipment type that suits the
// requested regiment kind (the quantity bonus follows the delivery level), and the player's "regiment reinforcements"
// request.
#include "units/rqreinf.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/random.hpp"
#include "core/recindex.hpp"
#include "core/rqcommon.hpp"
#include "io/listdlg.hpp"
#include "types.hpp"
#include "units/release.hpp"
#include "units/value.hpp"

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
void opeu_reinf_element(Game& game, uint16_t element, uint8_t kind, uint8_t side) {
  auto& data{ Data(game) };
  auto& loaded{ Record<Element>(data, element) };
  for (;;) {
    auto const index{ static_cast<std::uint8_t>(opeu_rand_bounded(TYPE_DRAW)) };
    if (!data.type_availability[index]) continue;
    auto const& type{ data.element_types[index] };
    auto const  category{ type.category };
    if (!Suits(kind, side, category)) continue;
    if (!(data.side_masks[data.current_side] & type.sides)) continue;
    if (data.type_availability[index] <= opeu_rand_bounded(100)) continue;
    loaded.type           = index;
    std::uint16_t amount  = data.request_capacities[category] / (opeu_max16(type.anti_tank, type.anti_personnel) / 20 + 1);
    amount               += (5 - data.delivery_level) * 6;
    loaded.quantity       = opeu_min16(opeu_rand_bounded(amount / 3) + (amount >> 1), data.request_capacities[category]);
    loaded.damage         = 0;
    return;
  }
}

// "Which regiment should we call in, sir?": cancels a pending regiment request, or files a new one for a free unit
// the player chooses, filling its free elements with fresh equipment.
void opeu_request_regiment(Game& game) {
  auto&      data{ Data(game) };
  auto const regiment{ std::to_underlying(RequestKind::REGIMENT) };
  auto const requested{ Bits(UnitFlag::REQUESTED) };
  if (opeu_request_pending(game, static_cast<uint8_t>(regiment))) {
    if (!opeu_request_cancel_prompt(game, Near(data, data.rq_reinforcements_text))) return;
    opeu_release_side_units(game, 0, static_cast<uint8_t>(requested));
    opeu_release_side_elements(game, 0, static_cast<uint8_t>(requested));
    Record<Side>(data, data.current_side_ptr).requests[regiment].kind = std::to_underlying(RequestKind::NONE);
  }
  auto const units{ static_cast<std::uint8_t>(opeu_release_side_units(game, Near(data, data.ai_unit_list), static_cast<uint8_t>(requested))) };
  if (!units) {
    opeu_request_no_more(game, Near(data, data.rq_regiment_text));
    return;
  }
  auto elements{ static_cast<std::uint8_t>(opeu_release_side_elements(game, Near(data, data.ai_element_list), static_cast<uint8_t>(requested))) };
  if (!elements) {
    opeu_request_no_more(game, Near(data, data.rq_battalion_text));
    return;
  }
  opeu_title_panel_begin(game, static_cast<const DialogHost*>(port_dialogs_host()));
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.rq_which_regiment_text), data.context_record2);
  data.rq_regiment_list.rows = static_cast<std::uint8_t>(units + 1);
  if (!opeu_list_dialog(game, Near(data, data.rq_regiment_list), static_cast<uint8_t>(LIST_LIMIT), static_cast<uint8_t>(LIST_FLAGS))) return;
  auto const    unit{ data.ai_unit_list[data.list_row] };
  auto const    slot{ static_cast<std::uint8_t>(opeu_unit_index(unit)) };
  std::uint16_t kind;
  if (opeu_category_0_to_3(static_cast<uint8_t>(slot)) || opeu_category_4_or_5(static_cast<uint8_t>(slot))) {
    opeu_army_title(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.rq_regiments_text));
    kind = opeu_delay_dialog(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.rq_regiment_kind_text), CHOSEN_KINDS, data.context_record2,
                             Near(data, data.rq_regiment_kind_menu));
    if (kind == 0xffff) return;
    kind--;
  } else if (opeu_category_6(static_cast<uint8_t>(slot)))
    kind = SLOT_6_KIND;
  else
    kind = OTHER_KIND;
  if (!opeu_confirm_ok(game, static_cast<const DialogHost*>(port_dialogs_host()), data.context_record2)) return;
  Record<Unit>(data, unit).flags = requested;
  auto const battalions{ data.ai_unit_element_counts[slot] };
  for (std::uint8_t row{ 0 }; row < battalions; ++row) {
    if (!opeu_rand_bounded(battalions) && row > 1) break;
    if (!elements) break;
    auto const near{ data.ai_element_list[row] };
    opeu_reinf_element(game, near, static_cast<std::uint8_t>(kind), static_cast<std::uint8_t>(opeu_rand_bounded(2)));
    auto& element{ Record<Element>(data, near) };
    element.morale     = static_cast<std::uint8_t>(opeu_rand_bounded(80));
    element.training   = static_cast<std::uint8_t>(opeu_rand_bounded(80));
    element.experience = static_cast<std::uint8_t>(opeu_rand_bounded(30));
    element.flags      = Bits(ElementFlag::REQUESTED);
    --elements;
  }
  FileRequest(game, { 0, static_cast<uint8_t>(static_cast<std::uint8_t>(kind)), static_cast<uint8_t>(regiment) });
  data.side_delays[data.current_side] = 1;
}
