#pragma once

#include <cstdint>

#include "data/image.hpp"

using opeu::game::FarPointer;

extern char       opeu_control_prompt[31];
extern char       opeu_control_ready[39];
extern uint8_t    opeu_control_text_padding;
extern FarPointer opeu_control_actions[11];
