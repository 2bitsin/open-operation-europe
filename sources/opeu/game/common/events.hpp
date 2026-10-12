// The service callbacks through which game routines leave their module: each takes the data segment, a service id and
// the argument words, and returns the service's result (AX, or DX:AX for the battle and UI services).
#pragma once

#include <cstdint>

using HostEvent      = std::uint16_t (*)(std::uint8_t* ds, std::uint16_t id,
                                         std::uint16_t const* args,
                                         std::uint16_t        count);
using HostWordsEvent = std::uint16_t (*)(
    std::uint8_t* ds, std::uint16_t kind, std::uint16_t* args,
    std::uint16_t count); // the service may write results into args
using HostService   = std::uint16_t (*)(std::uint8_t* ds, std::uint16_t call,
                                        std::uint16_t* words);
using HostNotify    = void (*)(std::uint8_t* ds, std::uint16_t id,
                               std::uint16_t const* args, std::uint16_t count);
using BattleEvent   = std::uint16_t (*)(std::uint8_t* ds, std::uint32_t service,
                                        std::uint16_t const* args,
                                        unsigned             count);
using BattleService = std::uint16_t (*)(std::uint8_t* ds, std::uint16_t id,
                                        std::uint16_t arg);
using PendingEvent  = std::uint16_t (*)(std::uint8_t* ds, std::uint16_t,
                                        std::uint16_t, std::uint16_t,
                                        std::uint16_t, std::uint16_t);
using SideColorEvent = void (*)(std::uint8_t* ds, std::uint16_t side);
using UiEvent = std::uint32_t (*)(std::uint8_t* ds, std::uint8_t* memory,
                                  std::uint16_t id, std::uint16_t const* args,
                                  std::uint16_t count);
