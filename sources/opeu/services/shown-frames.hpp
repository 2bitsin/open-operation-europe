// What the event pump shows. By default the screen the services draw on (vgalib's VGA model, where the opening draws),
// which the pump renders and presents itself; while a ShownFrames lives, the Display it names presents the frames
// (Display::Present: the game's screen on SDL_Renderer, opeu/entry.cpp).
#pragma once

#include "machine-display.hpp"

#include <functional>

namespace opeu::services::detail::pump {
class Pump;
}

namespace opeu::services::detail::shown_frames {

// The pump presents through `display` while this lives, then the services' screen again. The display outlives it.
class ShownFrames {
public:
  explicit ShownFrames(Display& display);
  ShownFrames(ShownFrames const&)                    = delete;
  ShownFrames(ShownFrames&&)                         = delete;
  auto operator=(ShownFrames const&) -> ShownFrames& = delete;
  auto operator=(ShownFrames&&) -> ShownFrames&      = delete;
  ~ShownFrames();

private:
  std::reference_wrapper<pump::Pump> _pump;
};

}

namespace opeu::services {
using detail::shown_frames::ShownFrames;
}
