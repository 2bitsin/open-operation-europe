// The information views' list builders and list row checks.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_list_side_objectives(uint8_t* ds, HostEvent event, uint16_t sp, uint8_t side);
uint8_t  opeu_list_unit_elements(uint8_t* ds, HostEvent event, uint16_t sp, uint16_t unit);
uint8_t  opeu_list_objective_stocks(uint8_t* ds, HostEvent event, uint16_t sp, uint16_t objective);
uint16_t opeu_element_kind_total(uint8_t* ds, HostEvent event, uint16_t sp, uint16_t unit, uint8_t category,
                                 uint16_t count);
uint8_t  opeu_view_objective_ok(uint8_t* ds, HostEvent event, uint16_t sp);
uint8_t  opeu_view_false(uint8_t* ds, HostEvent event, uint16_t sp);
uint8_t  opeu_view_group_ok(uint8_t* ds, HostEvent event, uint16_t sp);
uint8_t  opeu_view_unit_ok(uint8_t* ds, HostEvent event, uint16_t sp);
uint8_t  opeu_view_false_2(uint8_t* ds, HostEvent event, uint16_t sp);
uint8_t  opeu_view_false_3(uint8_t* ds, HostEvent event, uint16_t sp);
uint8_t  opeu_view_true(uint8_t* ds, HostEvent event, uint16_t sp);
uint8_t  opeu_list_element_types(uint8_t* ds, HostEvent event, uint16_t sp, uint16_t list, uint8_t mask,
                                 uint8_t category);

uint16_t svc_vwlist(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
