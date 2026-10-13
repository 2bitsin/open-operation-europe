#pragma once
// The demo's frames: tiles in the four hours' palettes, unit icons in both sides' colours, a blast animation,
// portraits stretched and flipped, the logo, a line of each font in pen colours, and a fade that breathes.

#include "opeu/assets/manifest.hpp"
#include "opeu/render/scene.hpp"

#include <cstddef>

namespace opeu::probes::render_demo::detail::demo_scene {

// Frames per fade: bright at 0, darkest halfway.
inline constexpr auto FADE_PERIOD{ std::size_t{ 120 } };

// The scene shown as frame `frame`; pens are the day palette's colours.
[[nodiscard]] auto DemoScene(std::size_t frame, assets::PaletteColours const& day) -> render::Scene;

}

namespace opeu::probes::render_demo {
using detail::demo_scene::DemoScene;
using detail::demo_scene::FADE_PERIOD;
}
