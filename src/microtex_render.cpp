/**
 * ASE MicroTeX Adapter — render_math implementation
 *
 * Translates the ASE-native call signature (cairomm context + utf-8 latex +
 * pixel coords) into MicroTeX's internal TeXRender/Graphics2D pipeline and
 * returns the bounding-box metrics as an ase::microtex::MathResult.
 *
 * Pipeline:
 *   1. latex char* ⇒ std::wstring (required by tex::LaTeX::parse)
 *   2. tex::LaTeX::parse(…) ⇒ TeXRender with laid-out box tree
 *   3. Graphics2D_cro wraps the caller's Cairo context
 *   4. TeXRender::draw() walks the box tree, emitting Cairo primitives
 *   5. width/height/depth/baseline are copied into MathResult
 *
 * Ownership: tex::LaTeX::parse() returns a heap-allocated TeXRender. ASE
 * forbids raw new/delete, so we wrap the pointer in a std::unique_ptr —
 * that keeps the delete inside the standard library's deleter and
 * guarantees cleanup even on exception.
 *
 * @module      ase-microtex-adapter
 * @layer       adapter (third-party isolation)
 */

#include <ase/microtex/render.hpp>
#include <ase/microtex/init.hpp>

#include "microtex_cairo_backend.hpp"
#include "latex.h"
#include "render.h"

#include <cstdint>
#include <memory>
#include <string>

namespace ase::microtex {

namespace {
// Convert a UTF-8 byte range to a wide-character string for MicroTeX.
// The validator forbids std::string_view, so we take raw char* + len.
std::wstring to_wstring(const char* latex, uint32_t latex_len) {
    // MicroTeX only checks ASCII structural characters in the common path
    // (backslashes, braces, digits); a straight char ⇒ wchar_t widen is
    // sufficient for the subset we ship. UTF-8 glyph references are routed
    // through MicroTeX's symbol tables which speak wchar_t natively.
    std::wstring out;
    out.reserve(latex_len);
    for (uint32_t i = 0; i < latex_len; ++i) {
        out.push_back(static_cast<wchar_t>(
            static_cast<unsigned char>(latex[i])));
    }
    return out;
}
}  // namespace

MathResult render_math(
    const Cairo::RefPtr<Cairo::Context>& cr,
    const char* latex,
    uint32_t latex_len,
    double x,
    double y,
    double font_size,
    bool is_display
) {
    // init() is idempotent and cheap. Clients should call it at startup,
    // but we guard here in case they forgot.
    init();

    const std::wstring wlatex = to_wstring(latex, latex_len);

    // MicroTeX LaTeX::parse signature:
    //   parse(tex, width, textSize, lineSpace, fg)
    // width == 0 means "auto" (no forced wrap).
    const int width = 0;
    const float text_size = static_cast<float>(font_size);
    const float line_space = static_cast<float>(font_size) * 0.2f;
    const tex::color fg = tex::black;

    // unique_ptr owns the heap-allocated TeXRender — cleanup happens in
    // the standard deleter on scope exit, no raw delete in this file.
    std::unique_ptr<tex::TeXRender> render{
        tex::LaTeX::parse(wlatex, width, text_size, line_space, fg)
    };

    // Display vs inline: both modes honour the supplied text_size. MicroTeX
    // selects DISPLAY style by default via its TeXRenderBuilder; inline
    // callers can shrink text_size upstream if they need a tighter run.
    (void)is_display;

    Graphics2D_cro g2(cr);
    render->draw(g2,
                 static_cast<int>(x),
                 static_cast<int>(y));

    MathResult result;
    result.width    = static_cast<double>(render->getWidth());
    result.height   = static_cast<double>(render->getHeight());
    result.depth    = static_cast<double>(render->getDepth());
    result.baseline = static_cast<double>(render->getBaseline());
    return result;
}

MathResult measure_math(
    const char* latex,
    uint32_t latex_len,
    double font_size,
    bool is_display
) {
    init();

    const std::wstring wlatex = to_wstring(latex, latex_len);

    const int width = 0;
    const float text_size = static_cast<float>(font_size);
    const float line_space = static_cast<float>(font_size) * 0.2f;
    const tex::color fg = tex::black;

    std::unique_ptr<tex::TeXRender> render{
        tex::LaTeX::parse(wlatex, width, text_size, line_space, fg)
    };
    (void)is_display;

    MathResult result;
    if (render) {
        result.width    = static_cast<double>(render->getWidth());
        result.height   = static_cast<double>(render->getHeight());
        result.depth    = static_cast<double>(render->getDepth());
        result.baseline = static_cast<double>(render->getBaseline());
    }
    return result;
}

}  // namespace ase::microtex
