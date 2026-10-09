cmake_minimum_required(VERSION 3.16)
project(opeu_port C CXX)

# Operation Europe, native build: the recovered C of src/main compiled for
# the host, linked with the port layer (port/link, port/services, port/core)
# and an SDL3 platform backend.

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 14)              # ymfm (the OPL2 chip)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
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
# Emscripten (emcmake cmake): SDL3 comes from emscripten's own port
# (--use-port=sdl3), which this emscripten release pins (6.0.10: SDL 3.4.2,
# checked against a SHA-512 by emcc). See "Web build" below.
if(EMSCRIPTEN)
  set(OPEU_SDL3 "")
  set(SDL3_FOUND TRUE)
else()
  find_package(SDL3 CONFIG QUIET)
endif()
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
if(EMSCRIPTEN)
  # the port is added as a compile and link option of the targets below
elseif(TARGET SDL3::SDL3)
  set(OPEU_SDL3 SDL3::SDL3)
elseif(TARGET SDL3::SDL3-static)
  set(OPEU_SDL3 SDL3::SDL3-static)
else()
  set(OPEU_SDL3 SDL3-static)
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# ---- the OPL2 sound chip: ymfm (BSD-3-Clause), port/third_party/ymfm ---------
# Pinned to aaronsgiles/ymfm commit 81aec25 (2026-07-27); see port/README.md.
# Emulation of the YM3812 sound chip is a hardware service behind the
set(OPEU_YMFM ${OPEU_PORT}/third_party/ymfm/src)
add_library(opeu_opl STATIC
  ${OPEU_PORT}/services/opl_ymfm.cpp
  ${OPEU_YMFM}/ymfm_opl.cpp
  ${OPEU_YMFM}/ymfm_adpcm.cpp
  ${OPEU_YMFM}/ymfm_pcm.cpp)
target_include_directories(opeu_opl PUBLIC ${OPEU_PORT}/services PRIVATE ${OPEU_YMFM})
if(EMSCRIPTEN)
  target_compile_options(opeu_opl PRIVATE -fno-exceptions)
endif()

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
target_link_libraries(opeu PRIVATE opeu_recovered opeu_opl ${OPEU_SDL3})
if(EMSCRIPTEN)
  set(OPEU_DEFAULT_DATA_DIR /data)
else()
  set(OPEU_DEFAULT_DATA_DIR ${OPEU_REPO}/binaries)
endif()
target_compile_definitions(opeu PRIVATE OPEU_DEFAULT_DATA_DIR="${OPEU_DEFAULT_DATA_DIR}" ${OPEU_SOURCE_FEATURES})

# ---- OPEN.EXE: the opening, run before MAIN (port/open) --------------------
# The recovered C of src/open compiled as is. OPEN and MAIN were separate DOS
# programs and reuse names, so every symbol a src/open module defines is
# renamed by the forced include port/open/open_names.h (generated by
# port/open/tools/gen_open_names.py); what src/open only references
# (MAIN's STRINGS/MEMORY routines, linked into OPEN.EXE unchanged) resolves
# to opeu_recovered. Link layer, image and start-up: port/open; OPEN-only
# assembly services: port/services/open_*.c (globbed above).
file(GLOB OPEU_OPEN_RECOVERED CONFIGURE_DEPENDS
  ${OPEU_SRC}/open/core/*.c
  ${OPEU_SRC}/open/data/*.c
  ${OPEU_SRC}/open/io/*.c)
add_library(opeu_open_recovered STATIC ${OPEU_OPEN_RECOVERED})
target_include_directories(opeu_open_recovered PRIVATE ${OPEU_SRC}/common)
if(MSVC)
  target_compile_options(opeu_open_recovered PRIVATE /FI${OPEU_PORT}/open/open_names.h)
else()
  target_compile_options(opeu_open_recovered PRIVATE -include ${OPEU_PORT}/open/open_names.h
    -Wall -Wextra -fno-strict-aliasing)
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    # otext.c's host branch #defines its DGROUP variables (opeu_text_x ...)
    # over the renames of the forced include; the module's own macro wins.
    target_compile_options(opeu_open_recovered PRIVATE -Wno-macro-redefined)
  endif()
endif()
target_link_libraries(opeu_open_recovered PRIVATE opeu_recovered)
file(GLOB_RECURSE OPEU_OPEN_PORT CONFIGURE_DEPENDS ${OPEU_PORT}/open/*.c)
target_sources(opeu PRIVATE ${OPEU_OPEN_PORT})
target_include_directories(opeu PRIVATE ${OPEU_PORT}/open)
target_link_libraries(opeu PRIVATE opeu_open_recovered)
if(NOT EMSCRIPTEN)
  # open_run: headless runs (key-skip, key at the title, headless default).
  # open_music: the opening's driver calls replayed on the original FMDRV.COM
  # (tests/validate_fmdrv.py's Unicorn harness; exit 77 = skipped without
  # unicorn); the register streams must be identical. Same Python cache
  # variable (and default) as the fmdriver_registers test below.
  if(EXISTS ${OPEU_REPO}/.venv/bin/python)
    set(OPEU_OPEN_TEST_PYTHON ${OPEU_REPO}/.venv/bin/python)
  else()
    set(OPEU_OPEN_TEST_PYTHON ${Python3_EXECUTABLE})
  endif()
  set(OPEU_TEST_PYTHON ${OPEU_OPEN_TEST_PYTHON} CACHE FILEPATH "Python with unicorn for the FM driver comparison")
  add_test(NAME open_run
    COMMAND ${Python3_EXECUTABLE} ${OPEU_PORT}/open/tests/check_open_run.py $<TARGET_FILE:opeu>
            ${CMAKE_CURRENT_BINARY_DIR}/open-run)
  set_tests_properties(open_run PROPERTIES TIMEOUT 900)
  add_test(NAME open_music
    COMMAND ${OPEU_TEST_PYTHON} ${OPEU_PORT}/open/tests/open_music_diff.py $<TARGET_FILE:opeu>
            $<TARGET_FILE:opeu_dump_open_bank> ${CMAKE_CURRENT_BINARY_DIR}/open-music)
  set_tests_properties(open_music PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 900)
  # open_provenance: every src/open module compiled, none of its routines
  # redefined under port/, the generated name map and image current.
  add_test(NAME open_provenance
    COMMAND ${Python3_EXECUTABLE} ${OPEU_PORT}/open/tests/check_open_provenance.py
            ${CMAKE_BINARY_DIR}/compile_commands.json)
endif()
# ---- end of OPEN.EXE ---------------------------------------------------------

# ---- web build (Emscripten) -------------------------------------------------
# emcmake cmake -S port -B port/build/web -DCMAKE_BUILD_TYPE=Release
# Produces opeu.html/.js/.wasm/.data. The original data files of OPEU_WEB_DATA
# (default binaries/) are packed into opeu.data at /gamedata; the DOS
# programs (*.EXE, *.COM), the DOS batch files and the eXoDOS marker are left
# out because the port does not use them. port/platform/web/pre.js copies
# /gamedata into /data (an IDBFS mount, so saved games persist in the
# browser's IndexedDB) before main() runs.
#
# Blocking waits: the game polls the keyboard/mouse/FM driver in loops; those
# reach plat_sleep_ms() and plat_present(), which call emscripten_sleep()
# under ASYNCIFY and so return to the browser. Every game routine reaches the
# pump through function-pointer service callbacks, so the whole module is
# instrumented (no ASYNCIFY_IGNORE_INDIRECT: it would drop exactly those
# paths). ASYNCIFY_STACK_SIZE holds the unwound native stack of the deepest
# wait (turn processing -> battle -> dialog -> key wait); 1 MiB leaves a wide
# margin. STACK_SIZE (4 MiB) gives the native C stack room for the same chains.
if(EMSCRIPTEN)
  set(OPEU_WEB_DATA ${OPEU_REPO}/binaries CACHE PATH "Original game directory packed into opeu.data")
  option(OPEU_WEB_JSPI "Use JSPI instead of ASYNCIFY (Chrome/Edge/Node only)" OFF)
  set(OPEU_SDL3_PORT --use-port=sdl3)
  target_compile_options(opeu PRIVATE ${OPEU_SDL3_PORT})
  file(GLOB OPEU_WEB_FILES LIST_DIRECTORIES false RELATIVE ${OPEU_WEB_DATA} ${OPEU_WEB_DATA}/*)
  set(OPEU_WEB_PRELOAD)
  set(OPEU_WEB_PRELOAD_DEPS)
  foreach(f IN LISTS OPEU_WEB_FILES)
    if(f MATCHES "\\.([Ee][Xx][Ee]|[Cc][Oo][Mm]|[Bb][Aa][Tt]|exo)$" OR f MATCHES " ")
      continue()
    endif()
    list(APPEND OPEU_WEB_PRELOAD "--preload-file=${OPEU_WEB_DATA}/${f}@/gamedata/${f}")
    list(APPEND OPEU_WEB_PRELOAD_DEPS ${OPEU_WEB_DATA}/${f})
  endforeach()
  if(OPEU_WEB_JSPI)
    set(OPEU_WEB_ASYNC -sJSPI=1)
  else()
    set(OPEU_WEB_ASYNC -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=1048576)
  endif()
  # Call-signature glue: see port/core/wasm_abi.c.
  set(OPEU_WASM_VOID_DEFS)
  foreach(r ai_08f8 ai_093c ai_0a5a ai_0c3c ai_0d82 ai_1064)
    list(APPEND OPEU_WASM_VOID_DEFS opeu_${r}=opeu_wasm_void_${r})
  endforeach()
  set_source_files_properties(${OPEU_SRC}/main/core/aiops5.c
    PROPERTIES COMPILE_DEFINITIONS "${OPEU_WASM_VOID_DEFS}")
  set(OPEU_WEB_DIR ${OPEU_PORT}/platform/web)
  target_link_options(opeu PRIVATE
    ${OPEU_SDL3_PORT}
    ${OPEU_WEB_ASYNC}
    -sENVIRONMENT=web,node
    -sALLOW_MEMORY_GROWTH=1
    -sINITIAL_MEMORY=33554432
    -sSTACK_SIZE=4194304
    -sEXIT_RUNTIME=1
    -sFORCE_FILESYSTEM=1
    -sEXPORTED_RUNTIME_METHODS=FS,addRunDependency,removeRunDependency
    -lidbfs.js
    -lnodefs.js
    --pre-js=${OPEU_WEB_DIR}/pre.js
    --shell-file=${OPEU_WEB_DIR}/shell.html
    ${OPEU_WEB_PRELOAD})
  set_target_properties(opeu PROPERTIES SUFFIX ".html"
    LINK_DEPENDS "${OPEU_WEB_DIR}/pre.js;${OPEU_WEB_DIR}/shell.html;${OPEU_WEB_PRELOAD_DEPS}")
endif()

# ---- tests ------------------------------------------------------------------
if(EMSCRIPTEN)
  return()
endif()
enable_testing()
add_executable(opeu_test_vgalib ${OPEU_PORT}/tests/test_vgalib.c ${OPEU_PORT}/services/vgalib.c)
target_include_directories(opeu_test_vgalib PRIVATE ${OPEU_PORT}/services)
add_test(NAME vgalib COMMAND opeu_test_vgalib)
add_executable(opeu_test_syslib_gfx ${OPEU_PORT}/tests/test_syslib_gfx.c
  ${OPEU_PORT}/services/syslib_gfx.c ${OPEU_PORT}/services/vgalib.c)
target_include_directories(opeu_test_syslib_gfx PRIVATE ${OPEU_PORT}/services)
add_test(NAME syslib_gfx COMMAND opeu_test_syslib_gfx)

# FM driver: the native driver's register stream against the original
# FMDRV.COM run under tests/validate_fmdrv.py's Unicorn harness (needs a
# Python with unicorn: the repository's .venv by default; exit 77 = skipped
# when it is missing). See port/tests/fmdriver_diff.py.
if(EXISTS ${OPEU_REPO}/.venv/bin/python)
  set(OPEU_TEST_PYTHON_DEFAULT ${OPEU_REPO}/.venv/bin/python)
else()
  set(OPEU_TEST_PYTHON_DEFAULT ${Python3_EXECUTABLE})
endif()
set(OPEU_TEST_PYTHON ${OPEU_TEST_PYTHON_DEFAULT} CACHE FILEPATH "Python with unicorn for the FM driver comparison")
add_executable(opeu_fmdriver_trace ${OPEU_PORT}/tests/fmdriver_trace.c ${OPEU_PORT}/services/fmdriver.c)
target_include_directories(opeu_fmdriver_trace PRIVATE ${OPEU_PORT}/services)
add_executable(opeu_dump_open_bank ${OPEU_PORT}/tests/dump_open_bank.c ${OPEU_SRC}/open/data/ofmusic.c)
target_include_directories(opeu_dump_open_bank PRIVATE ${OPEU_SRC}/common)
add_test(NAME fmdriver_registers
  COMMAND ${OPEU_TEST_PYTHON} ${OPEU_PORT}/tests/fmdriver_diff.py $<TARGET_FILE:opeu_fmdriver_trace>
          $<TARGET_FILE:opeu> --open-dump=$<TARGET_FILE:opeu_dump_open_bank>
          --json=${CMAKE_CURRENT_BINARY_DIR}/fmdriver-diff.json)
set_tests_properties(fmdriver_registers PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 900)

# The game's music in a headless run: title -> scenario briefing with
# --wav/--opl-log; the WAV must be audible and the key-ons must come when
# the game starts its songs. See port/tests/check_audio.py.
add_test(NAME fm_headless_audio
  COMMAND ${Python3_EXECUTABLE} ${OPEU_PORT}/tests/check_audio.py $<TARGET_FILE:opeu>
          ${OPEU_PORT}/tests/smoke-input.txt ${CMAKE_CURRENT_BINARY_DIR}/fm-smoke)
set_tests_properties(fm_headless_audio PROPERTIES TIMEOUT 600)
