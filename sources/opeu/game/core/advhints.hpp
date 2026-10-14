// The staff's operation hints and general tips.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <span>

uint16_t opeu_advice_situational_hint(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_general_tip(Game& game, std::span<std::uint8_t const> far_text);
uint16_t opeu_advice_operation_hints(Game& game, std::span<std::uint8_t const> far_text);
