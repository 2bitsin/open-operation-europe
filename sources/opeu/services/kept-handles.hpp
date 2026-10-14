#pragma once
// What a Display keeps for the game behind a handle (a saved rectangle, a battle cell, an overlay): each new value gets
// the next id, and past the capacity the oldest is let go. The game never sees what is kept, only the id.

#include "opeu/contract.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <utility>

namespace opeu::services::detail::kept_handles {

template <class _Id, class _Value>
class KeptHandles {
public:
  explicit KeptHandles(std::size_t const capacity) : _capacity{ capacity } {
    Expects(capacity > 0, "a store keeps at least one value");
  }

  // Keeps `value` under a new id, letting go of the oldest past the capacity.
  [[nodiscard]] auto Keep(_Value value) -> _Id {
    auto const id{ static_cast<_Id>(++_last) };
    _kept.insert_or_assign(id, std::move(value));
    if (_kept.size() > _capacity) _kept.erase(_kept.begin());
    return id;
  }
  [[nodiscard]] auto Find(_Id const id) -> std::optional<std::reference_wrapper<_Value>> {
    auto const found{ _kept.find(id) };
    if (found == _kept.end()) return std::nullopt;
    return std::ref(found->second);
  }
  [[nodiscard]] auto Find(_Id const id) const -> std::optional<std::reference_wrapper<_Value const>> {
    auto const found{ _kept.find(id) };
    if (found == _kept.end()) return std::nullopt;
    return std::cref(found->second);
  }
  // The value of `id`, no longer kept; none when it was not.
  [[nodiscard]] auto Release(_Id const id) -> std::optional<_Value> {
    auto found{ _kept.extract(id) };
    if (found.empty()) return std::nullopt;
    return std::move(found.mapped());
  }

private:
  std::size_t           _capacity;
  std::uint32_t         _last{};
  std::map<_Id, _Value> _kept;
};

}

namespace opeu::services {
using detail::kept_handles::KeptHandles;
}
