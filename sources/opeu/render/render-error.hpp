#pragma once
// Why something could not be drawn: SDL refused a call, or the pack could not answer.

#include "opeu/assets/pack-error.hpp"

#include <SDL3/SDL.h>

#include <expected>
#include <format>
#include <string>
#include <string_view>

namespace opeu::render::detail::render_error {

struct RenderError {
  std::string detail;

  auto operator==(RenderError const&) const -> bool = default;
};

template <class _Value>
using Rendered = std::expected<_Value, RenderError>;

[[nodiscard]] inline auto Failed(std::string detail) -> std::unexpected<RenderError> {
  return std::unexpected{ RenderError{ .detail = std::move(detail) } };
}

// SDL's own reason for the call that just failed.
[[nodiscard]] inline auto SdlFailed(std::string_view const call) -> std::unexpected<RenderError> {
  return Failed(std::format("{}: {}", call, SDL_GetError()));
}

[[nodiscard]] inline auto PackFailed(assets::PackError const& error) -> std::unexpected<RenderError> {
  return Failed(assets::Describe(error));
}

}

namespace opeu::render {
using detail::render_error::Failed;
using detail::render_error::PackFailed;
using detail::render_error::Rendered;
using detail::render_error::RenderError;
using detail::render_error::SdlFailed;
}
