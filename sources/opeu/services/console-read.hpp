#pragma once
// MS C's console read (getch) over the BIOS keyboard: the ASCII of the next key; a key with ASCII 0 (a function or
// cursor key) returns 0 now and its scan code on the next call. clock.cpp keeps the process's one until services::Host
// owns it (docs/platform-surface.md section 6).

#include <concepts>
#include <cstdint>
#include <optional>
#include <utility>

namespace opeu::services::detail::console_read {

class ConsoleRead {
public:
  // The next character; `read_key` waits for a key word (AH=scan code, AL=ASCII) when no scan code is pending.
  template <std::invocable _ReadKey>
  [[nodiscard]] auto Next(_ReadKey&& read_key) -> std::uint8_t {
    if (auto const scan{ std::exchange(_scan_pending, std::nullopt) }) return *scan;
    auto const key{ std::forward<_ReadKey>(read_key)() };
    auto const ascii{ static_cast<std::uint8_t>(key & 0xFF) };
    if (ascii == 0) _scan_pending = static_cast<std::uint8_t>(key >> 8);
    return ascii;
  }

private:
  std::optional<std::uint8_t> _scan_pending;
};

}

namespace opeu::services {
using detail::console_read::ConsoleRead;
}
