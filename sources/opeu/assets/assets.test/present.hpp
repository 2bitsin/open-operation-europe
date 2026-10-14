#pragma once
// An optional's value for a test to compare, or a default-made value when it is empty (the comparison then fails and
// says so) rather than undefined behaviour.

#include <optional>

namespace opeu::assets::test {

template <class _Value>
[[nodiscard]] auto Present(std::optional<_Value> const& value) -> _Value {
  return value.value_or(_Value{});
}

}
