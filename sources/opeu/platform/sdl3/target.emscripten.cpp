// The browser's side of target.hpp: an OpenGL canvas with its device pixels, the only two yields to the browser, and
// the save directory the page mounts.
#include "opeu/platform/sdl3/target.hpp"

#include <emscripten/emscripten.h>

#include <cstdint>

namespace opeu::platform::detail::target {

// The WebGL renderer needs an OpenGL window; given the flag up front, it does not recreate the window, which would leave
// the canvas's CSS size at 1px (SDL's destroy) and make the new window ignore the page's size.
auto ExtraWindowFlags() -> SDL_WindowFlags {
  return SDL_WINDOW_OPENGL;
}

// The page sizes the canvas to the largest integer multiple of the frame in device pixels (web/shell.html).
auto OwnsWindowSize() -> bool {
  return false;
}

// Keys go to the canvas (focused by the page's click-to-start), and only this backend yields to the browser.
auto PrepareHints() -> void {
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY, "0");
}

auto PaceFrames([[maybe_unused]] SDL_Renderer& renderer) -> void { }

// Return to the browser so it shows the frame and queues input.
auto AfterFrame() -> void {
  emscripten_sleep(0);
}

// Yield to the browser event loop (ASYNCIFY/JSPI unwinds the stack and resumes here from a timer).
auto Sleep(int const ms, bool const headless) -> void {
  if (ms >= 0 && !headless) emscripten_sleep(static_cast<std::uint32_t>(ms));
}

auto DefaultSaveDir() -> std::string {
  return "/saves"; // web/pre.js mounts IDBFS there
}

auto PrepareSaveDir([[maybe_unused]] std::string const& path) -> void { }

}
