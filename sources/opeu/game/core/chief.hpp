// The operations chief's portrait panel, and the messages, waits and number prompts shown beside it.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_ops_chief_show(uint8_t* ds, HostEvent event);
void     opeu_ops_chief_text(uint8_t* ds, HostEvent event, uint16_t text);
uint16_t opeu_number_prompt(uint8_t* ds, HostEvent event, uint16_t text, uint16_t low, uint16_t high, uint16_t value, uint16_t d, uint16_t buffer);
uint16_t opeu_ops_chief_number(uint8_t* ds, HostEvent event, uint16_t text, uint16_t low, uint16_t high, uint16_t value, uint16_t d, uint16_t buffer);
uint16_t opeu_ops_chief_wait(uint8_t* ds, HostEvent event, uint16_t text);

// Runs the services the module's routines call through their event.
uint16_t svc_chief(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
