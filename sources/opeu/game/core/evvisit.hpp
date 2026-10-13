// The commander's visit: a visitor chats with a fit commander on safe open ground and his ratings rise.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "types.hpp"

[[nodiscard]] auto opeu_visit_unit_safe(Game& game, opeu::game::Unit const& unit) -> bool;
void               opeu_visit_present(Game& game);
void               opeu_visit_event(Game& game);
