// Loads a scenario's record and its map from the contents of the scenario and map files.
#pragma once

#include <cstddef>
#include <cstdint>

auto opeu_load_scenario(std::uint8_t* ds, unsigned scenario,
                        std::uint8_t const* scedata, std::size_t scedata_size,
                        std::uint8_t const* mapinf, std::size_t mapinf_size)
    -> int;
