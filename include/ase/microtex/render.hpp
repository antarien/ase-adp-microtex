#pragma once

/**
 * ASE MicroTeX Adapter — Public Render API
 *
 * Renders a LaTeX math expression onto a Cairo drawing context using the
 * MicroTeX engine. This is the only surface clients need — the internal
 * tex::Graphics2D / tex::Font / tex::TextLayout inheritance is fully hidden.
 *
 * Clients must call ase::microtex::init() once at process startup before
 * any render_math() call — see init.hpp.
 *
 * @module      ase-microtex-adapter
 * @layer       adapter (third-party isolation)
 */

#include <cairomm/cairomm.h>
#include <cstdint>

namespace ase::microtex {

/**
 * Result of a math-expression render call.
 * Dimensions are in device pixels at the font size supplied to render_math().
 * `depth` is the number of pixels the glyph extends below the baseline.
 */
struct MathResult {
    double width    = 0.0;
    double height   = 0.0;
    double depth    = 0.0;
    double baseline = 0.0;
};

/**
 * Render a LaTeX math expression onto `cr` with its top-left corner at (x, y).
 *
 * @param cr          target Cairo context (from a Gtk draw signal)
 * @param latex       LaTeX source (UTF-8), e.g. "\\frac{a^2+b^2}{c}"
 * @param latex_len   length of latex in bytes (not counting a terminator)
 * @param x           top-left x in device pixels
 * @param y           top-left y in device pixels
 * @param font_size   font size in pixels
 * @param is_display  true for display math (larger, centered), false for inline
 * @return            bounding box + baseline metrics
 */
MathResult render_math(
    const Cairo::RefPtr<Cairo::Context>& cr,
    const char* latex,
    uint32_t latex_len,
    double x,
    double y,
    double font_size,
    bool is_display
);

/**
 * Measure a LaTeX math expression without drawing it.
 *
 * Used by inline math layout in paragraph rendering: callers reserve a
 * bounding box in their text layout (e.g. via a Pango shape attribute)
 * before drawing the actual glyphs at the laid-out position.
 *
 * @param latex       LaTeX source (UTF-8)
 * @param latex_len   length of latex in bytes
 * @param font_size   font size in pixels
 * @param is_display  true for display style, false for inline (text style)
 * @return            bounding box + baseline metrics (no drawing performed)
 */
MathResult measure_math(
    const char* latex,
    uint32_t latex_len,
    double font_size,
    bool is_display
);

}  // namespace ase::microtex
