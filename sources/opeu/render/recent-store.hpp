#pragma once
// A bounded store that lets go of what was used least recently: the renderer's texture caches.

#include "opeu/contract.hpp"

#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <utility>

namespace opeu::render::detail::recent_store {

struct CacheStatistics {
  std::size_t hits{};
  std::size_t misses{};
  std::size_t evictions{};

  auto operator==(CacheStatistics const&) const -> bool = default;
};

template <class _Key, class _Value>
class RecentStore {
public:
  explicit RecentStore(std::size_t const capacity) : _capacity{ capacity } {
    Expects(capacity > 0, "a store holds at least one value");
  }

  // The value kept for `key`, which becomes the most recent; counted as a hit, or a miss when there is none.
  [[nodiscard]] auto Find(_Key const& key) -> std::optional<_Value> {
    auto const found{ _slots.find(key) };
    if (found == _slots.end()) {
      ++_statistics.misses;
      return std::nullopt;
    }
    ++_statistics.hits;
    _recent.splice(_recent.begin(), _recent, found->second);
    return found->second->value;
  }

  // Keeps `value` as the most recent, letting go of the least recent past the capacity.
  auto Remember(_Key const& key, _Value value) -> void {
    if (auto const found{ _slots.find(key) }; found != _slots.end()) _recent.erase(found->second);
    _recent.push_front(Slot{ .key = key, .value = std::move(value) });
    _slots.insert_or_assign(key, _recent.begin());
    if (_recent.size() <= _capacity) return;
    _slots.erase(_recent.back().key);
    _recent.pop_back();
    ++_statistics.evictions;
  }

  [[nodiscard]] auto Holds(_Key const& key) const -> bool {
    return _slots.contains(key);
  }
  [[nodiscard]] auto Statistics() const noexcept -> CacheStatistics {
    return _statistics;
  }

private:
  struct Slot {
    _Key   key;
    _Value value;
  };
  using Recent = std::list<Slot>;

  std::size_t                                            _capacity;
  Recent                                                 _recent; // most recently used first
  std::map<_Key, typename Recent::iterator, std::less<>> _slots;
  CacheStatistics                                        _statistics;
};

}

namespace opeu::render {
using detail::recent_store::CacheStatistics;
using detail::recent_store::RecentStore;
}
