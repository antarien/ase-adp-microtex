#pragma once

/**
 * ASE MicroTeX Adapter — Umbrella Header
 *
 * Single include that pulls the entire public surface:
 *   - ase::adp::microtex::init() / shutdown()  (lifecycle)
 *   - ase::adp::microtex::render_math()        (draw a formula to Cairo)
 *   - ase::adp::microtex::MathResult           (bounding-box return value)
 *
 * The adapter isolates MicroTeX's tex::* inheritance-based backend API
 * from client code. Clients never include MicroTeX headers directly.
 *
 * @module      ase-adp-microtex
 * @layer       adapter (third-party isolation)
 */

#include <ase/adp/microtex/init.hpp>
#include <ase/adp/microtex/render.hpp>
