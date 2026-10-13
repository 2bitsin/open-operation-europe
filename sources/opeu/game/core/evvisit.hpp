// The commander's visit: a visitor chats with a fit commander on safe open ground and his ratings rise.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_visit_unit_safe(Game& game, uint16_t unit);
void     opeu_visit_person_boost(Game& game, uint16_t person);
void     opeu_visit_conversation(Game& game, uint8_t side);
void     opeu_visit_present(Game& game);
void     opeu_visit_event(Game& game);
