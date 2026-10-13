// The information views' list builders and list row checks.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_list_side_objectives(Game& game, HostEvent event, uint8_t side);
uint8_t  opeu_list_unit_elements(Game& game, HostEvent event, uint16_t unit);
uint8_t  opeu_list_objective_stocks(Game& game, HostEvent event, uint16_t objective);
uint16_t opeu_element_kind_total(Game& game, HostEvent event, uint16_t unit, uint8_t category, uint16_t count);
uint8_t  opeu_view_objective_ok(Game& game, HostEvent event);
uint8_t  opeu_view_false(Game& game, HostEvent event);
uint8_t  opeu_view_group_ok(Game& game, HostEvent event);
uint8_t  opeu_view_unit_ok(Game& game, HostEvent event);
uint8_t  opeu_view_false_2(Game& game, HostEvent event);
uint8_t  opeu_view_false_3(Game& game, HostEvent event);
uint8_t  opeu_view_true(Game& game, HostEvent event);
uint8_t  opeu_list_element_types(Game& game, HostEvent event, uint16_t list, uint8_t mask, uint8_t category);

uint16_t svc_vwlist(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
