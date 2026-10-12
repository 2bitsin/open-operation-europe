// The staff's "Regiment Status" advice.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_advice_regiment_order_finishing(uint8_t*       ds,
                                              const uint8_t* far_text,
                                              HostEvent event, uint16_t sp);
uint16_t opeu_advice_regiment_supply_shortage(uint8_t*       ds,
                                              const uint8_t* far_text,
                                              HostEvent event, uint16_t sp);
uint16_t opeu_advice_division_commander_condition(uint8_t*       ds,
                                                  const uint8_t* far_text,
                                                  HostEvent event, uint16_t sp);
uint16_t opeu_advice_regiment_damage_or_comms(uint8_t*       ds,
                                              const uint8_t* far_text,
                                              HostEvent event, uint16_t sp);
uint16_t opeu_advice_regiment_fatigue_morale(uint8_t*       ds,
                                             const uint8_t* far_text,
                                             HostEvent event, uint16_t sp);
uint16_t opeu_advice_regiment_nothing(uint8_t* ds, const uint8_t* far_text,
                                      HostEvent event, uint16_t sp);
uint16_t opeu_advice_regiment_status(uint8_t* ds, const uint8_t* far_text,
                                     HostEvent event, uint16_t sp);

// Runs the services of the module's checks.
uint16_t svc_advregt(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
