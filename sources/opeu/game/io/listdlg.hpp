// The list selection dialogs.
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_commander_list_dialog(Game& game, HostWordsEvent event, std::uint16_t window, std::uint8_t limit, std::uint8_t flags) -> std::uint16_t;
auto opeu_list_dialog(Game& game, HostWordsEvent event, std::uint16_t window, std::uint8_t limit, std::uint8_t flags) -> std::uint16_t;

// The module's services: the list calls (io/listui.hpp).
auto svc_listdlg(Game& game, std::uint16_t kind, std::uint16_t* args, std::uint16_t count) -> std::uint16_t;
