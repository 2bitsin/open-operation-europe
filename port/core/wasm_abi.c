/* WebAssembly call-signature glue (web build only).
 *
 * aiops5.c declares six AI routines of aiops3.c/aiops4.c as returning void
 * (aiops5.c:169-171) while their definitions return uint16_t; aiops5 ignores
 * the value.  Native ABIs tolerate the mismatch (the return register is just
 * not read); WebAssembly checks function types at link time and wasm-ld
 * turns such a call into a trap stub (and the module then fails Binaryen's
 * validation).  The build therefore compiles aiops5.c with each of these
 * names mapped to a void forwarder below (CMakeLists.txt, EMSCRIPTEN only);
 * the forwarder calls the recovered routine and drops its value, which is
 * what the native call does.  No game logic; src/ is unchanged.
 */
#ifdef __EMSCRIPTEN__
#include <stdint.h>
#include "recovered.h"

#define OPEU_WASM_VOID_FORWARDER(name)          \
    void opeu_wasm_void_##name(uint8_t *ds);    \
    void opeu_wasm_void_##name(uint8_t *ds)     \
    {                                           \
        (void)opeu_##name(ds);                  \
    }

OPEU_WASM_VOID_FORWARDER(ai_08f8)
OPEU_WASM_VOID_FORWARDER(ai_093c)
OPEU_WASM_VOID_FORWARDER(ai_0a5a)
OPEU_WASM_VOID_FORWARDER(ai_0c3c)
OPEU_WASM_VOID_FORWARDER(ai_0d82)
OPEU_WASM_VOID_FORWARDER(ai_1064)
#endif
