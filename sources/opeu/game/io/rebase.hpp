// Converts the record indices a scenario or saved game stores into near pointers.
#pragma once

#include <cstdint>

void opeu_rebase_scenario(std::uint8_t* ds, std::uint16_t extended);
