// Dispatch by a code enum (the code slots of common/data-segment.hpp, the presenters of common/presenter.hpp): a
// constexpr table of (enumerator, handler) rows in the module that owns the enum, searched for the enumerator. Until M4
// the enumerators are far addresses, not indexes, so the table is searched rather than indexed.
#pragma once

#include "opeu/contract.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace opeu::game::detail::dispatch {

// One table: each enumerator the slot may hold, with the handler that runs for it.
template <typename _Key, typename _Handler, std::size_t ROWS>
using DispatchTable = std::array<std::pair<_Key, _Handler>, ROWS>;

// A table written as its rows, one per line; the row count is the rows'.
template <typename _Key, typename _Handler, std::size_t ROWS>
[[nodiscard]] consteval auto DispatchRows(std::pair<_Key, _Handler> (&&rows)[ROWS]) -> DispatchTable<_Key, _Handler, ROWS> {
  return std::to_array(std::move(rows));
}

// The handler the table holds for key; a key the table does not hold breaks the contract (it is never undefined).
template <typename _Key, typename _Handler, std::size_t ROWS>
[[nodiscard]] constexpr auto HandlerOf(DispatchTable<_Key, _Handler, ROWS> const& table, _Key key) -> _Handler {
  auto const row{ std::ranges::find(table, key, &std::pair<_Key, _Handler>::first) };
  if (row == table.end()) Unreachable(key);
  return row->second;
}

// Runs the handler the table holds for key with the arguments; its result is the handler's.
template <typename _Key, typename _Handler, std::size_t ROWS, typename... _Arguments>
constexpr auto Dispatch(DispatchTable<_Key, _Handler, ROWS> const& table, _Key key, _Arguments&&... arguments) -> decltype(auto) {
  return HandlerOf(table, key)(std::forward<_Arguments>(arguments)...);
}
} // namespace opeu::game::detail::dispatch

namespace opeu::game {
using detail::dispatch::Dispatch;
using detail::dispatch::DispatchRows;
using detail::dispatch::DispatchTable;
using detail::dispatch::HandlerOf;
} // namespace opeu::game
