// The routines the fixture's services call.
#pragma once

#include <cstdint>
#include <span>

void opeu_show(std::uint8_t* ds, std::uint16_t x, std::uint8_t y);
auto opeu_lookup(std::uint8_t* ds, std::int16_t id) -> std::int16_t;
auto opeu_width(std::uint8_t* ds, unsigned pixels) -> std::uint16_t;
void opeu_format(std::uint8_t* ds, std::span<std::uint16_t const> words);
void opeu_pair(std::uint8_t* ds, std::uint16_t first, std::uint16_t second);
auto opeu_tick(std::uint8_t* ds) -> std::uint16_t;
void opeu_host(std::uint8_t* ds, void const* host);
