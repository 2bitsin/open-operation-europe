// The arrival message: the arriving unit's name, built in a buffer on the caller's stack, in the side's format.
#include "core/arrshow.hpp"
#include "common/events.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/unitname.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "io/soundcue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t REQUEST_BUFFER{ offsetof(DataSegment, request_buffer) };
constexpr std::uint16_t ARRIVAL_FORMAT{ 75 }; // the message id of the arrival format
constexpr std::uint16_t FIRST_SIDE_TOKEN{ 54 };
constexpr std::uint16_t SECOND_SIDE_TOKEN{ 53 };
} // namespace

// The arriving unit's name goes into a 32-byte buffer of the routine's own for the format service.
void opeu_arrival_present(Game& game) {
  TextBuffer<0x20> unit_name;
  auto&            data{ Data(game) };
  opeu_request_wait(game);
  opeu_daily_context(game, data.current_side);
  auto const          name{ opeu_unit_name(game, data.arrival_unit, unit_name) };
  auto const          side{ data.arrival_side };
  std::uint16_t const token{ side == 0 ? FIRST_SIDE_TOKEN : SECOND_SIDE_TOKEN };
  auto const          format{ opeu_message_lookup(game, ARRIVAL_FORMAT) };
  opeu_request_format(game, REQUEST_BUFFER, std::array<FormatArgument, 5>{ format, token, data.side_names[side], token, name });
  opeu_control_message(game, REQUEST_BUFFER, data.request_person);
}
