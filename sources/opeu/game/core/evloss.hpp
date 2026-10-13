// Hourly unit losses: partisan and resistance strikes, and the losses north of row 10.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_loss_unit_weak(uint8_t* ds, HostEvent event, uint16_t unit);
void     opeu_loss_present_a(uint8_t* ds, HostEvent event);
void     opeu_loss_event_a(uint8_t* ds, HostEvent event);
uint16_t opeu_loss_unit_low(uint8_t* ds, uint16_t unit);
void     opeu_loss_present_b(uint8_t* ds, HostEvent event);
void     opeu_loss_event_b(uint8_t* ds, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_evloss(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
