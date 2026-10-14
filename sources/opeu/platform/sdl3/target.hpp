#pragma once
// What differs between the browser and a native target, one implementation per target: target.emscripten.cpp (the
// browser) and target.native.cpp (macOS, Linux, Windows), chosen by buildutil's platform tags. The browser's waits are
// polling loops that never return to a main loop, so its build hands control back to the browser at two places only:
// after each presented frame and in the pump's idle sleep (docs/port.md "The browser build").

#include <SDL3/SDL.h>

#include <string>

namespace opeu::platform::detail::target {

// The window flags the target adds to a resizable window (the browser: an OpenGL window with the canvas's device
// pixels).
[[nodiscard]] auto ExtraWindowFlags() -> SDL_WindowFlags;
// SDL hints the target needs before SDL_Init (the browser: keys go to the canvas, no automatic yields).
auto PrepareHints() -> void;
// Paces the renderer's presents (native: vsync; the browser paces frames itself).
auto PaceFrames(SDL_Renderer& renderer) -> void;
// After a presented frame: the browser shows it and queues input; native targets do nothing.
auto AfterFrame() -> void;
// The pump's idle sleep of `ms` milliseconds; a headless browser run never yields.
auto Sleep(int ms, bool headless) -> void;
// The save directory when no --saves was given.
[[nodiscard]] auto DefaultSaveDir() -> std::string;
// Makes sure the save directory exists (the browser's is mounted by the page).
auto PrepareSaveDir(std::string const& path) -> void;

}
