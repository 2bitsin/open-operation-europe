// The paratroop drop: counting it down, flying it, and calling it off.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_para_clear(Game& game);
uint16_t opeu_para_ready(Game& game, HostEvent event);
uint16_t opeu_para_drop_due(Game& game, HostEvent event);
void     opeu_para_drop_present(Game& game, HostEvent event);
void     opeu_para_drop_event(Game& game, HostEvent event, uint16_t segment);

// The services the drop routines are called with.
uint16_t svc_paradrop(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
