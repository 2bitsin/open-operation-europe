// The Paratroops command (core/airdrop).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

extern uint16_t opeu_airdrop_code_segment;
uint8_t         opeu_air_mark_regiment(Game& game, HostEvent event);
uint8_t         opeu_air_assign_troops(Game& game, HostEvent event);
uint8_t         opeu_air_mark_battalion(Game& game, HostEvent event);
uint8_t         opeu_air_paratroop_type(Game& game, HostEvent event);
uint16_t        opeu_air_choose_forces(Game& game, HostEvent event);
uint8_t         opeu_air_drop_delay(Game& game, HostEvent event, uint8_t hours);
uint16_t        opeu_air_drop_order(Game& game, HostEvent event, uint8_t x, uint8_t y, uint8_t hours);
void            opeu_air_drop_command(Game& game, HostEvent event);

uint16_t svc_airdrop(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
