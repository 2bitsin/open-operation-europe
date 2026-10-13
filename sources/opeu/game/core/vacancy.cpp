// The vacant command posts of a side: its three staff roles and its four divisions.
#include "core/vacancy.hpp"
#include "common/game.hpp"
#include "core/people.hpp"
#include "data-segment.hpp"
#include "types.hpp"

using namespace opeu::game;

// The current side's first unfilled post: staff roles 1..3 give 0..2, divisions 0..3 give 3..6; 255 when every post
// is filled.
uint8_t opeu_find_vacant_post(Game const& game) {
  uint8_t const side{ Data(game).current_side };
  for (uint8_t role = 1; role <= 3; ++role)
    if (!opeu_find_side_record(game, side, role)) return role - 1;
  for (uint8_t group = 0; group < GROUPS_PER_SIDE; ++group)
    if (!opeu_find_group_record(game, side, group)) return group + 3;
  return 0xff;
}

// Relieves the side's general of staff role kind. With no such general the write lands on near pointer 0's record,
// in the segment's first bytes, which nothing reads.
void opeu_clear_staff_role(Game& game, uint8_t side, uint8_t kind) {
  auto& data{ Data(game) };
  Record<Person>(data, opeu_find_side_record(game, side, kind)).role = 0;
}

// Relieves the commander of the side's division group (the same null-record write when there is none).
void opeu_clear_division_command(Game& game, uint8_t side, uint8_t group) {
  auto& data{ Data(game) };
  Record<Person>(data, opeu_find_group_record(game, side, group)).assignment = 0;
}
