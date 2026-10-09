# Require() feeds both find_package and the conan graph: one source of truth.
Require(GTest VERSION ">=1.17.0" TEST CONAN gtest
        COMPONENTS gtest gtest_main gmock gmock_main)

# The port's platform layer (port/platform/sdl3), found by its own
# find_package(SDL3 CONFIG) once conan has put this on the prefix path.
# Pinned to the 3.4 series the web build's emscripten port carries (SDL
# 3.4.2), so desktop and browser run the same SDL minor. The web build
# keeps --use-port=sdl3 and never asks conan.
Require(SDL3 VERSION "3.4.16" CONAN sdl PLATFORM Linux Windows Macos)

Scan_subdirectories()
