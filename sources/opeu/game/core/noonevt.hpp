// The dated noon events, their actions and the leaders' speeches.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_noon_no_action(uint8_t* ds, const uint8_t* far_text, HostEvent event,
                         uint16_t sp);
void opeu_noon_block_axis_regts(uint8_t* ds, const uint8_t* far_text,
                                HostEvent event, uint16_t sp);
void opeu_noon_block_axis_sup_bns(uint8_t* ds, const uint8_t* far_text,
                                  HostEvent event, uint16_t sp);
void opeu_noon_block_axis_bns_regts(uint8_t* ds, const uint8_t* far_text,
                                    HostEvent event, uint16_t sp);
void opeu_noon_allow_axis_bns_regts(uint8_t* ds, const uint8_t* far_text,
                                    HostEvent event, uint16_t sp);
void opeu_noon_raise_axis_morale(uint8_t* ds, const uint8_t* far_text,
                                 HostEvent event, uint16_t sp);
void opeu_noon_lower_axis_morale(uint8_t* ds, const uint8_t* far_text,
                                 HostEvent event, uint16_t sp);
void opeu_noon_lower_allied_morale(uint8_t* ds, const uint8_t* far_text,
                                   HostEvent event, uint16_t sp);
void opeu_noon_axis_morale_blockade(uint8_t* ds, const uint8_t* far_text,
                                    HostEvent event, uint16_t sp);
void opeu_code_0e57_00dc(uint8_t* ds, const uint8_t* far_text, HostEvent event,
                         uint16_t sp);
void opeu_code_0e57_010e(uint8_t* ds, const uint8_t* far_text, HostEvent event,
                         uint16_t sp);
void opeu_code_0e57_0140(uint8_t* ds, const uint8_t* far_text, HostEvent event,
                         uint16_t sp);
uint16_t opeu_noon_portrait(uint8_t* ds, const uint8_t* far_text,
                            HostEvent event, uint16_t sp, uint16_t person,
                            uint16_t show);
void     opeu_noon_speech(uint8_t* ds, const uint8_t* far_text, HostEvent event,
                          uint16_t sp, uint16_t speech, uint16_t person);
void     opeu_noon_leader(uint8_t* ds, const uint8_t* far_text, HostEvent event,
                          uint16_t sp, uint8_t code, uint8_t line);
void opeu_turn_noon_events(uint8_t* ds, const uint8_t* far_text,
                           HostEvent event, uint16_t sp, uint16_t code_segment);

// The services of the noon events (NoonService in noonevt.cpp).
uint16_t svc_noonevt(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
