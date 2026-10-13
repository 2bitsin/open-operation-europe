// The message that tells the player why a filed request could not be carried out.
#include "core/reqerror.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {

// The first of the three reasons' messages: the objective is lost, no free place, no room for the delivery.
constexpr uint16_t FIRST_REASON_MESSAGE{ 61 };

} // namespace

// Formats the reason the request (invalid_request) failed into the request buffer and shows it from the side's chief
// of staff. When no side is played by a person the message closes by itself.
void opeu_request_error_present(Game& game) {
  auto& data{ Data(game) };
  opeu_pending_prepare(game);
  auto const& request{ Record<RequestSlot>(data, data.invalid_request) };
  auto const  buffer{ static_cast<uint16_t>(offsetof(DataSegment, request_buffer)) };
  auto const  name{ data.request_kind_names[request.kind] };
  auto const  format{ opeu_message_lookup(game, FIRST_REASON_MESSAGE + data.request_error) };
  if (data.request_error == 0)
    opeu_request_format(game, std::array<std::uint16_t, 4>{ buffer, format, request.target, name });
  else
    opeu_request_format(game, std::array<std::uint16_t, 3>{ buffer, format, name });
  opeu_control_message(game, buffer, data.request_template);
  if (data.side_control[1] == SIDE_COMPUTER && data.side_control[0] == SIDE_COMPUTER) opeu_pending_finish(game, 3);
}
