// Looking up a battle message: the far text of the message id is copied into the shared message buffer after the
// current side's daily context is selected.
#include "core/msglook.hpp"
#include "common/events.hpp"
#include "core/textout.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/context.hpp"
#include "core/memory.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace { } // namespace

// Returns the near pointer of the message buffer. Every id is looked up: the game's range test compares the truth
// value of 0 < id with 135 and so always passes, and the dispatch error is never reported.
uint16_t opeu_message_lookup(Game& game, int16_t id) {
  auto const& data{ Data(game) };
  opeu_daily_context(game, static_cast<uint8_t>(data.current_side));
  auto const& text{ data.message_table[static_cast<uint16_t>(id)] };
  opeu_string_copy_far(game, offsetof(DataSegment, message_buffer), port_far(text.segment, 0), text.offset);
  return offsetof(DataSegment, message_buffer);
}
