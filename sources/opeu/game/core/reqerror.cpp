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
#include "link/glue.hpp"
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
  opeu_pending_prepare(game, svc_soundcue);
  auto const& request{ Record<RequestSlot>(data, data.invalid_request) };
  auto const  buffer{ static_cast<uint16_t>(offsetof(DataSegment, request_buffer)) };
  auto const  name{ data.request_kind_names[request.kind] };
  auto const  format{ opeu_message_lookup(game, svc_msglook, static_cast<short>(FIRST_REASON_MESSAGE + data.request_error)) };
  if (data.request_error == 0)
    opeu_request_format(
        game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(format), static_cast<std::uint16_t>(request.target), static_cast<std::uint16_t>(name) });
  else
    opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(format), static_cast<std::uint16_t>(name) });
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), buffer, data.request_template);
  if (data.side_control[1] == SIDE_COMPUTER && data.side_control[0] == SIDE_COMPUTER) opeu_pending_finish(game, static_cast<uint8_t>(3));
}

auto port_routine_opeu_request_error_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_request_error_present(game);
  return 0;
}
