cmake_minimum_required(VERSION 3.16)
project(opeu_port C)

# Operation Europe, native build: the recovered C of src/main compiled for
# the host, linked with the port layer (port/link, port/services, port/core)
# and an SDL3 platform backend.

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

get_filename_component(OPEU_REPO ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)
set(OPEU_SRC ${OPEU_REPO}/src CACHE PATH "Recovered reference tree to compile (default: this checkout's src)")

# Two routines gained host branches after the first port snapshot: the AI
# path fill 19df:006e (unitai.c, replacing the opeu_path_fill_hook boundary)
# and the movement reachability fill 32e6:00d8 (movecost.c). The link layer
# calls them when the compiled reference defines them.
file(READ ${OPEU_SRC}/main/core/unitai.c OPEU_UNITAI_TEXT)
file(READ ${OPEU_SRC}/main/map/movecost.c OPEU_MOVECOST_TEXT)
set(OPEU_SOURCE_FEATURES)
if(OPEU_UNITAI_TEXT MATCHES "uint16_t opeu_path_fill\\(uint8_t \\*ds")
  list(APPEND OPEU_SOURCE_FEATURES OPEU_HAVE_PATH_FILL=1)
endif()
if(OPEU_MOVECOST_TEXT MATCHES "uint16_t opeu_move_reachable\\(DS_ARG WORDS_ARG")
  list(APPEND OPEU_SOURCE_FEATURES OPEU_HAVE_MOVE_REACHABLE=1)
endif()
message(STATUS "Recovered reference: ${OPEU_SRC} (${OPEU_SOURCE_FEATURES})")
set(OPEU_PORT ${CMAKE_CURRENT_SOURCE_DIR})

# ---- SDL3: installed package first, otherwise a static build from source ----
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
  message(STATUS "SDL3 not found; fetching SDL release-3.2.10 (static)")
  include(FetchContent)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.10
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(SDL3)
endif()
if(TARGET SDL3::SDL3)
  set(OPEU_SDL3 SDL3::SDL3)
elseif(TARGET SDL3::SDL3-static)
  set(OPEU_SDL3 SDL3::SDL3-static)
else()
  set(OPEU_SDL3 SDL3-static)
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# ---- the recovered game: every host branch of src/main ---------------------
file(GLOB OPEU_RECOVERED CONFIGURE_DEPENDS
  ${OPEU_SRC}/main/core/*.c
  ${OPEU_SRC}/main/data/*.c
  ${OPEU_SRC}/main/io/*.c
  ${OPEU_SRC}/main/map/*.c
  ${OPEU_SRC}/main/units/*.c)
add_library(opeu_recovered STATIC ${OPEU_RECOVERED})
target_include_directories(opeu_recovered PRIVATE ${OPEU_SRC}/common ${OPEU_SRC}/main/include)
# The recovered sources are compiled as the validators compile them, minus
# -Werror (warnings stay visible; the reference is frozen).
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(opeu_recovered PRIVATE -Wall -Wextra -fno-strict-aliasing)
endif()

# ---- generated link glue ----------------------------------------------------
file(GLOB_RECURSE OPEU_LINK_SOURCES CONFIGURE_DEPENDS ${OPEU_PORT}/link/*.c)
file(GLOB OPEU_SERVICE_SOURCES CONFIGURE_DEPENDS ${OPEU_PORT}/services/*.c)
set(OPEU_GEN ${CMAKE_CURRENT_BINARY_DIR}/gen)
add_custom_command(
  OUTPUT ${OPEU_GEN}/link_table.c
  COMMAND ${Python3_EXECUTABLE} ${OPEU_PORT}/tools/gen_link_table.py ${OPEU_GEN}/link_table.c
  DEPENDS ${OPEU_LINK_SOURCES} ${OPEU_SERVICE_SOURCES} ${OPEU_PORT}/tools/gen_link_table.py ${OPEU_PORT}/gen/symbols.json
  COMMENT "Generating the routine link table")
add_custom_command(
  OUTPUT ${OPEU_GEN}/callbacks.h ${OPEU_GEN}/link_defaults.c
  COMMAND ${Python3_EXECUTABLE} ${OPEU_PORT}/tools/gen_callbacks.py ${CMAKE_C_COMPILER} ${OPEU_GEN}
  DEPENDS ${OPEU_LINK_SOURCES} ${OPEU_PORT}/tools/gen_callbacks.py ${OPEU_PORT}/gen/callback_types.json
  COMMENT "Generating module callback declarations")

# ---- the game executable ----------------------------------------------------
file(GLOB OPEU_CORE_SOURCES CONFIGURE_DEPENDS ${OPEU_PORT}/core/*.c)
set(OPEU_PLATFORM_SOURCES
  ${OPEU_PORT}/platform/png_write.c
  ${OPEU_PORT}/platform/sdl3/platform_sdl3.c)
add_executable(opeu
  ${OPEU_PORT}/main.c
  ${OPEU_CORE_SOURCES}
  ${OPEU_SERVICE_SOURCES}
  ${OPEU_LINK_SOURCES}
  ${OPEU_PORT}/gen/data_image.c
  ${OPEU_PORT}/gen/module_data.c
  ${OPEU_GEN}/link_table.c
  ${OPEU_GEN}/link_defaults.c
  ${OPEU_PLATFORM_SOURCES})
target_include_directories(opeu PRIVATE
  ${OPEU_PORT}/include ${OPEU_PORT}/link ${OPEU_PORT}/services ${OPEU_PORT}/platform
  ${OPEU_PORT}/gen ${OPEU_GEN} ${OPEU_SRC}/common)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(opeu PRIVATE -Wall -Wextra -Wno-unused-parameter)
elseif(MSVC)
  target_compile_options(opeu PRIVATE /W3)
endif()
target_link_libraries(opeu PRIVATE opeu_recovered ${OPEU_SDL3})
target_compile_definitions(opeu PRIVATE OPEU_DEFAULT_DATA_DIR="${OPEU_REPO}/binaries" ${OPEU_SOURCE_FEATURES})

# ---- tests ------------------------------------------------------------------
enable_testing()
add_executable(opeu_test_vgalib ${OPEU_PORT}/tests/test_vgalib.c ${OPEU_PORT}/services/vgalib.c)
target_include_directories(opeu_test_vgalib PRIVATE ${OPEU_PORT}/services)
add_test(NAME vgalib COMMAND opeu_test_vgalib)
add_executable(opeu_test_syslib_gfx ${OPEU_PORT}/tests/test_syslib_gfx.c
  ${OPEU_PORT}/services/syslib_gfx.c ${OPEU_PORT}/services/vgalib.c)
target_include_directories(opeu_test_syslib_gfx PRIVATE ${OPEU_PORT}/services)
add_test(NAME syslib_gfx COMMAND opeu_test_syslib_gfx)
