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
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/microtex/init.hpp>
#include <ase/adp/microtex/render.hpp>
