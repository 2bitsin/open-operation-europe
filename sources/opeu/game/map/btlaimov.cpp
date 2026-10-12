// The computer's battle unit profile and its elements' move and attack orders.
//
// The attack keeps its two coordinate bytes on the DS stack, where the random cell choice writes them; the near, mid
// and far attacks take sp, the modeled SP at their entry, and run the attack on the frame below.
#include "types.hpp"
#include "map/btlaimov.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "map/btlaiunt.hpp"
#include "map/btlunit.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "map/btlaiact.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t NONE{ 0xff };
// ElementType::mode above this fires from afar.
constexpr uint8_t RANGED_MODE{ 1 };
// ElementType::speed above this makes fast tanks.
constexpr uint8_t FAST_SPEED{ 60 };
// The attack's frame below the callers' sp.
constexpr uint16_t ATTACK_FRAME{ 10 };
constexpr uint16_t X_AT{ 2 };
constexpr uint16_t Y_AT{ 4 };

// The original's counters frame: the six kind counters, then the loop index, the element cursor and, at 10, the
// element count, which battle_ai_kind reaches for category 6.
constexpr unsigned KIND_COUNT{ 6 };
constexpr unsigned FAST_TANKS{ 1 };
constexpr unsigned INFANTRY_KIND{ 2 }; // from battle_ai_kind
constexpr unsigned SPECIAL_KIND{ 4 };  // from battle_ai_kind
constexpr unsigned RANGED{ 5 };
constexpr unsigned ELEMENT_COUNT{ 10 };
} // namespace

// The profile of the unit's elements (the spotted ones, or all of them), indexing battle_ai_actions.
uint8_t opeu_battle_ai_profile(uint8_t* ds, BattleEvent event, uint16_t unit,
                               uint16_t all) {
  auto&                   data{ Data(ds) };
  std::array<uint8_t, 12> counters{};
  for (auto& element : Elements(data, Record<Unit>(data, unit))) {
    if (!opeu_battle_element_active(ds, event, Near(data, element)))
      continue;
    if ((element.flags & Bits(ElementFlag::SPOTTED)) || all) {
      auto const& type{ data.element_types[element.type] };
      if (type.mode > RANGED_MODE)
        ++counters[RANGED];
      else if (type.speed > FAST_SPEED &&
               type.category == Bits(Category::TANKS))
        ++counters[FAST_TANKS];
      else {
        auto const kind{ Record<uint8_t>(
            data, static_cast<uint16_t>(Near(data, data.battle_ai_kind) +
                                        type.category)) };
        if (kind < KIND_COUNT || kind == ELEMENT_COUNT)
          ++counters[kind];
      }
    }
    ++counters[ELEMENT_COUNT];
  }
  auto const count{ counters[ELEMENT_COUNT] };
  if (counters[SPECIAL_KIND])
    return 4;
  if (counters[RANGED] && count - counters[RANGED] < 4)
    return 2;
  if (count * 2 / 3 <= counters[INFANTRY_KIND] || counters[INFANTRY_KIND] > 2)
    return 0;
  if (count * 2 / 3 <= counters[FAST_TANKS] || counters[FAST_TANKS] > 2)
    return 1;
  return 3;
}

void opeu_battle_ai_move_to(uint8_t* ds, BattleEvent event, uint8_t x,
                            uint8_t y, uint16_t element) {
  auto& position{ PositionOf(Data(ds), element) };
  position.goal_x = x;
  position.goal_y = y;
  position.state  = Bits(BattleState::BLITZ);
  opeu_battle_disengage(ds, event, element);
  opeu_battle_set_facing(ds, event, element, NONE);
}

// frame: the routine's frame on the DS stack; the random cell is written there.
void opeu_battle_ai_attack(uint8_t* ds, BattleEvent event, uint16_t element,
                           uint16_t unit, uint16_t target, uint16_t frame) {
  if (target) {
    auto& position{ PositionOf(Data(ds), element) };
    position.target = target;
    position.state  = Bits(BattleState::BLITZ);
    opeu_battle_engage(ds, event, element);
    opeu_battle_set_facing(ds, event, element, NONE);
    return;
  }
  auto const x_at{ Local(frame, X_AT) };
  auto const y_at{ Local(frame, Y_AT) };
  opeu_battle_ai_random_cell(ds, event, x_at, y_at, element, unit);
  opeu_battle_ai_move_to(ds, event, Byte(ds, x_at), Byte(ds, y_at), element);
}

void opeu_battle_ai_attack_near(uint8_t* ds, BattleEvent event,
                                uint16_t element, uint16_t unit, uint16_t sp) {
  opeu_battle_ai_attack(ds, event, element, unit,
                        opeu_battle_ai_pick_target(ds, event, element, unit, 0),
                        Local(sp, ATTACK_FRAME));
}

void opeu_battle_ai_attack_mid(uint8_t* ds, BattleEvent event, uint16_t element,
                               uint16_t unit, uint16_t sp) {
  opeu_battle_ai_attack(ds, event, element, unit,
                        opeu_battle_ai_pick_target(ds, event, element, unit, 1),
                        Local(sp, ATTACK_FRAME));
}

void opeu_battle_ai_attack_far(uint8_t* ds, BattleEvent event, uint16_t element,
                               uint16_t unit, uint16_t sp) {
  opeu_battle_ai_attack(ds, event, element, unit,
                        opeu_battle_ai_pick_target(ds, event, element, unit, 2),
                        Local(sp, ATTACK_FRAME));
}

uint16_t svc_btlaimov(uint8_t* ds, uint32_t service, const uint16_t* args,
                      unsigned int count) {
  return svc_battle_ai(ds, service, args, count);
}
