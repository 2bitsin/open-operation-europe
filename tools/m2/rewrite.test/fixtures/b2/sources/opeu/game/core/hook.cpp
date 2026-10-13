// The fixture's code pointers: a far pointer to the show routine keeps its handler, and a routine whose address is
// taken keeps its battle event.
#include "common/events.hpp"

#include <cstdint>

struct FarPointer {
  std::uint16_t offset;
  std::uint16_t segment;
};

constexpr std::uint16_t SHOW_SEGMENT{ 0x0100 };

FarPointer const show_hook{ 0x0020, SHOW_SEGMENT };

void          opeu_hooked(Game& game, BattleEvent event);
using Hooked = void (*)(Game& game, BattleEvent event);

Hooked const hooked{ opeu_hooked };

void opeu_hooked(Game& game, BattleEvent event) {
  event(game, 0, nullptr, 0);
}
