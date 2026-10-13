// Looking up a battle message into the shared message buffer.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_message_lookup(Game& game, int16_t id);
