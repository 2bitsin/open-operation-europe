// The request menu's shared pieces: its service table and the helpers its choices use.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include <cstdint>

void     opeu_request_no_more(Game& game, uint16_t what);
uint16_t opeu_request_pending(Game const& game, uint8_t kind);
uint16_t opeu_request_mode_is(Game const& game, uint8_t mode);
uint16_t opeu_request_cancel_prompt(Game& game, uint16_t what);
void     opeu_request_finish(Game& game);
void     opeu_request_file(Game& game, uint16_t request);
uint16_t opeu_side_has_city(Game const& game);

namespace opeu::game::detail::rqcommon {
// Files the request from a copy in a DS reserve of its own.
auto FileRequest(Game& game, RequestSlot request) -> void;
} // namespace opeu::game::detail::rqcommon

namespace opeu::game {
using detail::rqcommon::FileRequest;
} // namespace opeu::game
