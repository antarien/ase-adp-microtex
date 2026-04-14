/**
 * ASE MicroTeX Adapter — init/shutdown implementation
 *
 * Wraps MicroTeX's one-shot resource loader (tex::LaTeX::init) with an
 * idempotent ASE entry point. The resource path is baked in at build time
 * by CMake via ASE_MICROTEX_RES_PATH.
 *
 * @module      ase-adp-microtex
 * @layer       adapter (third-party isolation)
 */

#include <ase/adp/microtex/init.hpp>

#include "latex.h"  // tex::LaTeX

#ifndef ASE_MICROTEX_RES_PATH
#error "ASE_MICROTEX_RES_PATH must be injected by CMake — see adapter/ase-adp-microtex/CMakeLists.txt"
#endif

namespace ase::adp::microtex {

namespace {
// Lazy-initialized flag: true after tex::LaTeX::init() has run successfully.
bool& initialized_flag() {
    static bool flag = false;
    return flag;
}
}  // namespace

void init() {
    bool& initialized = initialized_flag();
    if (initialized) {
        return;
    }
    tex::LaTeX::init(std::string{ASE_MICROTEX_RES_PATH});
    initialized = true;
}

void shutdown() {
    bool& initialized = initialized_flag();
    if (!initialized) {
        return;
    }
    tex::LaTeX::release();
    initialized = false;
}

}  // namespace ase::adp::microtex
