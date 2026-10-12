// The personnel orders menu (turn action 4) and its list and picture callbacks.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void    opeu_personnel_finish(uint8_t* ds, HostEvent event, uint16_t sp,
                              uint16_t segment);
uint8_t opeu_commander_always(uint8_t* ds, HostEvent event, uint16_t sp,
                              uint16_t segment, uint8_t n);
void    opeu_rest_thanks(uint8_t* ds, HostEvent event, uint16_t sp,
                         uint16_t segment, uint16_t person);
void    opeu_assign_confirm(uint8_t* ds, HostEvent event, uint16_t sp,
                            uint16_t segment, uint16_t person);
uint8_t opeu_division_toggle(uint8_t* ds, HostEvent event, uint16_t sp,
                             uint16_t segment, uint8_t n);
void    opeu_division_commander_menu(uint8_t* ds, HostEvent event, uint16_t sp,
                                     uint16_t segment);
uint8_t opeu_idle_commanders_fill(uint8_t* ds, HostEvent event, uint16_t sp,
                                  uint16_t segment);
void    opeu_commander_assign(uint8_t* ds, HostEvent event, uint16_t sp,
                              uint16_t segment);
void    opeu_commander_rest(uint8_t* ds, HostEvent event, uint16_t sp,
                            uint16_t segment);
void    opeu_turn_action_4(uint8_t* ds, HostEvent event, uint16_t sp,
                           uint16_t segment);

// The services the personnel routines are called with.
uint16_t svc_persmenu(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
