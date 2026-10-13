// The fixture's numbered services and the routines that call them.
#pragma once

#include "common/events.hpp"

#include <cstdint>

enum AlphaService : std::uint16_t {
  SHOW,
  LOOKUP,
  WIDTH,
  FORMAT,
  PAIR,
  TWICE,
  WRITE_BACK,
  HOST,
  FAR_ACTION,
};

uint16_t svc_alpha(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
uint16_t svc_beta(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
void     opeu_alpha_run(std::uint8_t* ds, HostEvent event, std::uint16_t x, std::int16_t signed_word, unsigned wide);
void     opeu_alpha_main(std::uint8_t* ds);
