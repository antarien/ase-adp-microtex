#pragma once

/**
 * ASE MicroTeX Adapter — Init/Shutdown API
 *
 * MicroTeX loads its font, glyph, and formula-mapping resources lazily on
 * first use via tex::LaTeX::init(). Clients must call ase::microtex::init()
 * once during application startup (before the first render_math() call) so
 * the resources are loaded from the adapter's bundled res/ tree.
 *
 * shutdown() releases the MicroTeX context and is safe to call even if
 * init() was never invoked.
 *
 * @module      ase-microtex-adapter
 * @layer       adapter (third-party isolation)
 */

namespace ase::microtex {

/**
 * Load MicroTeX resources from the adapter's bundled res/ tree.
 * Idempotent — safe to call multiple times.
 */
void init();

/**
 * Release the MicroTeX static context and free loaded resources.
 * Safe to call even if init() was never invoked.
 */
void shutdown();

}  // namespace ase::microtex
