#pragma once
// One JSON object built field by field, for the records the extractor prints (one per line).

#include <concepts>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

namespace opeu::probes::extract_assets::detail::json {

[[nodiscard]] auto Quoted(std::string_view text) -> std::string;

// A range of numbers or of already-encoded values as a JSON array.
template <std::ranges::input_range Values>
[[nodiscard]] auto Array(Values&& values) -> std::string {
  auto items{ std::forward<Values>(values) | std::views::transform([](auto const& value) {
                if constexpr (std::convertible_to<decltype(value), std::string_view>) {
                  return std::string{ value };
                } else {
                  return std::format("{}", +value);
                }
              })
              | std::views::join_with(std::string_view{ "," }) | std::ranges::to<std::string>() };
  return "[" + items + "]";
}

class Object {
public:
  auto Text(std::string_view const name, std::string_view const value) -> Object& {
    return Raw(name, Quoted(value));
  }
  auto Number(std::string_view const name, std::integral auto const value) -> Object& {
    return Raw(name, std::format("{}", +value));
  }
  auto Flag(std::string_view const name, bool const value) -> Object& {
    return Raw(name, value ? "true" : "false");
  }
  auto Raw(std::string_view name, std::string_view encoded) -> Object&;

  [[nodiscard]] auto Encoded() const -> std::string {
    return "{" + _fields + "}";
  }

private:
  std::string _fields;
};

}

namespace opeu::probes::extract_assets::json {
using detail::json::Array;
using detail::json::Object;
using detail::json::Quoted;
}
