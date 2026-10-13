// The routine behind the rows service; its signature needs Rows.
#pragma once

#include "common/rows.hpp"

auto opeu_rows_first(std::uint8_t* ds, Rows const* previous) -> std::uint16_t;
