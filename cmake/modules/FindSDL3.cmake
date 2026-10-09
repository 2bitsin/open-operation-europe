# The Emscripten SDK's SDL3 port (Require(SDL3 ... SYSTEM PLATFORM
# Emscripten)), prepared by cmake/browser.cmake, which puts this directory
# on CMAKE_MODULE_PATH (a sub-directory of cmake/ is not auto-included).
# After scooby-engine's cmake/modules/FindSDL3.cmake; the SDK port installs
# no config package, so the target is an interface carrying the port flag.
if(EXISTS "${EMSCRIPTEN_SYSROOT}/include/SDL3/SDL_version.h")
  set(SDL3_INCLUDE_DIRS "${EMSCRIPTEN_SYSROOT}/include")
  file(READ "${SDL3_INCLUDE_DIRS}/SDL3/SDL_version.h" SDL_VERSION_HEADER)
  foreach(PART MAJOR_VERSION MINOR_VERSION MICRO_VERSION)
    string(REGEX MATCH "#define SDL_${PART} +([0-9]+)" MATCH
      "${SDL_VERSION_HEADER}")
    set(SDL_${PART} "${CMAKE_MATCH_1}")
  endforeach()
  set(SDL3_VERSION
    "${SDL_MAJOR_VERSION}.${SDL_MINOR_VERSION}.${SDL_MICRO_VERSION}")
endif()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL3 REQUIRED_VARS SDL3_INCLUDE_DIRS
  VERSION_VAR SDL3_VERSION)
if(SDL3_FOUND AND NOT TARGET SDL3::SDL3)
  add_library(SDL3::SDL3 INTERFACE IMPORTED)
  set_target_properties(SDL3::SDL3 PROPERTIES
    INTERFACE_COMPILE_OPTIONS "--use-port=sdl3"
    INTERFACE_LINK_OPTIONS "--use-port=sdl3")
endif()
