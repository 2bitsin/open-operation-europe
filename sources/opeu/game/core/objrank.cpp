// The current side's ranked objective list (side_entry_values): the active objectives grouped by who holds them and
// on whose side of the front they lie, each group ordered by the influence on its cell.
#include "core/objrank.hpp"
#include "common/forces.hpp"
#include "core/daylist.hpp"
#include "data-segment.hpp"
#include "map/cellflag.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include <algorithm>
#include <array>
#include <span>

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// From this delivery level on, a computer side ranks objectives of a higher level first among equal influence.
constexpr uint8_t RANKED_DELIVERY{ 3 };

// The side's ranked list: OBJECTIVE_COUNT near objective pointers, the unused tail 0.
auto Entries(DataSegment& data, uint8_t side) -> std::span<uint16_t, OBJECTIVE_COUNT> {
  return std::span<uint16_t, OBJECTIVE_COUNT>{ data.side_entry_values + side * OBJECTIVE_COUNT, OBJECTIVE_COUNT };
}

// One pass of the ranking: the objectives held by `owner` on cells where `influence` dominates. Side 1's objectives
// are ordered by descending influence, side 0's by ascending.
struct RankPass {
  uint8_t owner;
  uint8_t influence;
};

using RankOrder = std::array<RankPass, 4>;

// The order of the passes for each side, offensive or not.
constexpr RankOrder SIDE_0_OFFENSIVE{
  { { 0, 1 }, { 1, 0 }, { 1, 1 }, { 0, 0 } }
};
constexpr RankOrder SIDE_0_DEFENSIVE{
  { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } }
};
constexpr RankOrder SIDE_1_OFFENSIVE{
  { { 1, 0 }, { 0, 1 }, { 0, 0 }, { 1, 1 } }
};
constexpr RankOrder SIDE_1_DEFENSIVE{
  { { 1, 1 }, { 1, 0 }, { 0, 1 }, { 0, 0 } }
};

} // namespace

// Inserts objective at entries[last], carrying it through entries[first..last-1]: each slot keeps the objective
// with the smaller influence (descending: the larger). From RANKED_DELIVERY on a computer side, a slot gives way
// only to an objective of at least its level.
void opeu_rank_entry_insert(Game& game, uint8_t side, uint8_t first, uint8_t last, uint16_t objective, uint8_t descending) {
  auto&      data{ Data(game) };
  auto const entries{ Entries(data, side) };
  auto const ranked{ !data.side_control[data.current_side] && data.delivery_level >= RANKED_DELIVERY };
  for (auto i = first; i < last; i++) {
    auto const  slot{ entries[i] };
    auto const& carried{ Record<Objective>(data, objective) };
    auto const& held{ Record<Objective>(data, slot) };
    auto const  carried_influence{ opeu_cell_influence(game, carried.x, carried.y) };
    auto const  held_influence{ opeu_cell_influence(game, held.x, held.y) };
    auto const  out_of_order{ descending ? held_influence < carried_influence : held_influence > carried_influence };
    if (out_of_order && (!ranked || opeu_objective_level(game, slot) <= opeu_objective_level(game, objective))) {
      entries[i] = objective;
      objective  = slot;
    }
  }
  entries[last] = objective;
}

// Ranks the current side's objectives in four passes (RankOrder) and clears the rest of its list.
void opeu_rank_side_entries(Game& game) {
  auto&       data{ Data(game) };
  auto const  offensive{ data.side_stance[data.current_side] == Bits(Stance::OFFENSIVE) };
  auto const  side{ static_cast<uint8_t>(data.current_side == 0 ? 0 : 1) };
  auto const& order{ side == 0 ? (offensive ? SIDE_0_OFFENSIVE : SIDE_0_DEFENSIVE) : (offensive ? SIDE_1_OFFENSIVE : SIDE_1_DEFENSIVE) };
  uint8_t     next{ 0 };
  for (auto const& pass : order) {
    auto const first{ next };
    for (auto& objective : data.objectives) {
      if (objective.active && opeu_objective_side(game, Near(data, objective)) == pass.owner && opeu_cell_influence_side(game, objective.x, objective.y) == pass.influence)
        opeu_rank_entry_insert(game, side, first, next++, Near(data, objective), pass.owner);
    }
  }
  auto const entries{ Entries(data, data.current_side) };
  std::fill(entries.begin() + next, entries.end(), 0);
}

// A computer side with a ranked first objective has each of its divisions plan its tasks again.
void opeu_side_groups_retarget(Game& game, uint8_t side) {
  auto& data{ Data(game) };
  if (data.side_control[side] == SIDE_PLAYED || !Entries(data, side)[0]) return;
  for (uint8_t i = 0; i < GROUPS_PER_SIDE; i++) GroupOf(data, side, i).flags |= Bits(GroupFlag::RETARGET);
}

// Re-ranks the current side's objectives; an offensive computer side whose ranking changed retargets its divisions.
void opeu_turn_selected_begin(Game& game) {
  auto&                                 data{ Data(game) };
  auto const                            side{ data.current_side };
  std::array<uint16_t, OBJECTIVE_COUNT> before{};
  std::ranges::copy(Entries(data, side), before.begin());
  opeu_rank_side_entries(game);
  if (data.side_control[side] == SIDE_COMPUTER && data.side_stance[side] == Bits(Stance::OFFENSIVE) && !std::ranges::equal(Entries(data, side), before)) opeu_side_groups_retarget(game, side);
}
