// The commander's visit: a visitor chats with a fit commander on safe open ground and his ratings rise.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_visit_unit_safe(uint8_t* ds, uint16_t unit);
void     opeu_visit_person_boost(uint8_t* ds, uint16_t person);
void     opeu_visit_conversation(uint8_t* ds, HostEvent event, uint8_t side);
void     opeu_visit_present(uint8_t* ds, HostEvent event);
void     opeu_visit_event(uint8_t* ds, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_evvisit(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
