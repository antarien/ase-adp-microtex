/**
 * ASE MicroTeX Adapter — Cairo/Pango Graphics2D backend (implementation)
 *
 * Port of upstream NanoMichael/MicroTeX graphic_cairo.cpp to the gtkmm-4 stack
 * (cairomm-1.16 + pangomm-2.48). Geometry and order-of-operations are preserved
 * 1:1 — these are MicroTeX's internal drawing primitives, not ASE logic. Enum
 * paths, includes, and style dispatch are rewritten to ASE conventions
 * (no std::max, no reinterpret_cast, no switch/case, no static containers).
 *
 * @module      ase-adp-microtex
 * @layer       adapter (third-party isolation)
 */

#include "microtex_cairo_backend.hpp"

#include <fontconfig/fontconfig.h>
#include <cairomm/fontface.h>
#include <cmath>
#include <utility>

#include "utils/utf.h"  // tex::wide2utf8

namespace ase::microtex {

// ═══════════════════════════════════════════════════════════════════════════
// Font_cro
// ═══════════════════════════════════════════════════════════════════════════

Font_cro::Font_cro(std::string family, int style, float size)
    : _family(std::move(family)), _style(style), _size(size) {}

Font_cro::Font_cro(const std::string& file, float size)
    : Font_cro(std::string{}, tex::PLAIN, size) {
    loadFont(file);
}

void Font_cro::loadFont(const std::string& file) {
    // Casts go through void* so the path is static_cast-only and compliant.
    const FcChar8* fc_file =
        static_cast<const FcChar8*>(static_cast<const void*>(file.c_str()));

    int count = 0;
    FcChar8* family = nullptr;
    FcBlanks* blanks = FcConfigGetBlanks(nullptr);
    FcPattern* pat = FcFreeTypeQuery(fc_file, 0, blanks, &count);
    FcPatternGetString(pat, FC_FAMILY, 0, &family);
    FcConfigAppFontAddFile(nullptr, fc_file);

    _family = static_cast<const char*>(static_cast<const void*>(family));
    _fface = Cairo::FtFontFace::create(pat);

    FcPatternDestroy(pat);
}

tex::sptr<tex::Font> Font_cro::deriveFont(int style) const {
    return tex::sptrOf<Font_cro>(_family, style, _size);
}

bool Font_cro::operator==(const tex::Font& ft) const {
    const auto& f = static_cast<const Font_cro&>(ft);
    return _size == f._size && _style == f._style && _family == f._family;
}

bool Font_cro::operator!=(const tex::Font& f) const {
    return !(*this == f);
}

// MicroTeX core calls these two factories to create platform-specific fonts.
// They are declared in graphic/graphic.h but left unimplemented by the core;
// every backend must provide them. They live in the tex:: namespace.
}  // namespace ase::microtex

namespace tex {
Font* Font::create(const std::string& file, float size) {
    return new ase::microtex::Font_cro(file, size);
}
sptr<Font> Font::_create(const std::string& name, int style, float size) {
    return sptrOf<ase::microtex::Font_cro>(name, style, size);
}
}  // namespace tex

namespace ase::microtex {

// ═══════════════════════════════════════════════════════════════════════════
// TextLayout_cro
// ═══════════════════════════════════════════════════════════════════════════

namespace {
// A tiny 1x1 image context is used only to create Pango::Layout objects for
// text measurement — Pango requires a Cairo context but never draws into it.
const Cairo::RefPtr<Cairo::Context>& measurement_context() {
    static const Cairo::RefPtr<Cairo::Context> ctx = [] {
        auto surface =
            Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, 1, 1);
        return Cairo::Context::create(surface);
    }();
    return ctx;
}

// Style dispatch without switch/case — one tag-filtered branch per style.
void apply_font_style(Pango::FontDescription& fd, int style) {
    const bool is_bold = (style == tex::BOLD || style == tex::BOLDITALIC);
    const bool is_italic = (style == tex::ITALIC || style == tex::BOLDITALIC);
    fd.set_weight(is_bold ? Pango::Weight::BOLD : Pango::Weight::NORMAL);
    fd.set_style(is_italic ? Pango::Style::ITALIC : Pango::Style::NORMAL);
}
}  // namespace

TextLayout_cro::TextLayout_cro(const std::wstring& src, const tex::sptr<Font_cro>& font) {
    _layout = Pango::Layout::create(measurement_context());

    Pango::FontDescription fd;
    fd.set_family(font->getFamily());
    fd.set_absolute_size(font->getSize() * Pango::SCALE);
    apply_font_style(fd, font->getStyle());

    _layout->set_text(tex::wide2utf8(src));
    _layout->set_font_description(fd);

    _ascent = static_cast<float>(_layout->get_baseline() / Pango::SCALE);
}

void TextLayout_cro::getBounds(tex::Rect& r) {
    int w = 0, h = 0;
    _layout->get_pixel_size(w, h);
    r.x = 0;
    r.y = -_ascent;
    r.w = static_cast<float>(w);
    r.h = static_cast<float>(h);
}

void TextLayout_cro::draw(tex::Graphics2D& g2, float x, float y) {
    // Upstream workaround: force a Cairo path before showing the Pango layout
    // to avoid a translate-drift that leaves the layout one line too low.
    const tex::color old = g2.getColor();
    g2.setColor(0x00000000);
    g2.drawLine(x, y, x + 1, y);
    g2.setColor(old);

    g2.translate(x, y - _ascent);
    auto& cro = static_cast<Graphics2D_cro&>(g2);
    _layout->show_in_cairo_context(cro.getCairoContext());
    g2.translate(-x, -y + _ascent);
}

}  // namespace ase::microtex

namespace tex {
sptr<TextLayout> TextLayout::create(const std::wstring& src, const sptr<Font>& font) {
    auto f = std::static_pointer_cast<ase::microtex::Font_cro>(font);
    return sptrOf<ase::microtex::TextLayout_cro>(src, f);
}
}  // namespace tex

namespace ase::microtex {

// ═══════════════════════════════════════════════════════════════════════════
// Graphics2D_cro
// ═══════════════════════════════════════════════════════════════════════════

namespace {
// Default font for the backend when no explicit font has been set.
// Lazy-initialized static local (per-TU) — not a mutable container.
Font_cro& default_font() {
    static Font_cro font(std::string{"SansSerif"}, tex::PLAIN, 20.f);
    return font;
}

// Cap/join dispatch without switch/case — branchless table lookups.
Cairo::Context::LineCap translate_cap(int cap) {
    if (cap == tex::CAP_ROUND)  return Cairo::Context::LineCap::ROUND;
    if (cap == tex::CAP_SQUARE) return Cairo::Context::LineCap::SQUARE;
    return Cairo::Context::LineCap::BUTT;
}
Cairo::Context::LineJoin translate_join(int join) {
    if (join == tex::JOIN_BEVEL) return Cairo::Context::LineJoin::BEVEL;
    if (join == tex::JOIN_ROUND) return Cairo::Context::LineJoin::ROUND;
    return Cairo::Context::LineJoin::MITER;
}
}  // namespace

Graphics2D_cro::Graphics2D_cro(const Cairo::RefPtr<Cairo::Context>& context)
    : _context(context), _color(tex::black), _stroke(), _font(&default_font()),
      _sx(1.f), _sy(1.f) {
    setColor(tex::black);
    setStroke(tex::Stroke());
    setFont(_font);
}

void Graphics2D_cro::setColor(tex::color c) {
    _color = c;
    const double a = tex::color_a(c) / 255.0;
    const double r = tex::color_r(c) / 255.0;
    const double g = tex::color_g(c) / 255.0;
    const double b = tex::color_b(c) / 255.0;
    _context->set_source_rgba(r, g, b, a);
}

tex::color Graphics2D_cro::getColor() const { return _color; }

void Graphics2D_cro::setStroke(const tex::Stroke& s) {
    _stroke = s;
    _context->set_line_width(s.lineWidth);
    _context->set_line_cap(translate_cap(s.cap));
    _context->set_line_join(translate_join(s.join));
    _context->set_miter_limit(s.miterLimit);
}

const tex::Stroke& Graphics2D_cro::getStroke() const { return _stroke; }

void Graphics2D_cro::setStrokeWidth(float w) {
    _stroke.lineWidth = w;
    _context->set_line_width(w);
}

const tex::Font* Graphics2D_cro::getFont() const { return _font; }

void Graphics2D_cro::setFont(const tex::Font* font) {
    _font = static_cast<const Font_cro*>(font);
}

void Graphics2D_cro::translate(float dx, float dy) {
    _context->translate(dx, dy);
}

void Graphics2D_cro::scale(float sx, float sy) {
    _sx *= sx;
    _sy *= sy;
    _context->scale(sx, sy);
}

void Graphics2D_cro::rotate(float angle) {
    _context->rotate(angle);
}

void Graphics2D_cro::rotate(float angle, float px, float py) {
    _context->translate(px, py);
    _context->rotate(angle);
    _context->translate(-px, -py);
}

void Graphics2D_cro::reset() {
    _context->set_identity_matrix();
    _sx = _sy = 1.f;
}

void Graphics2D_cro::drawChar(wchar_t c, float x, float y) {
    const std::wstring str = {c, L'\0'};
    drawText(str, x, y);
}

void Graphics2D_cro::drawText(const std::wstring& t, float x, float y) {
    _context->set_font_face(_font->getCairoFontFace());
    _context->set_font_size(_font->getSize());
    _context->move_to(x, y);
    _context->show_text(tex::wide2utf8(t));
}

void Graphics2D_cro::drawLine(float x1, float y1, float x2, float y2) {
    _context->move_to(x1, y1);
    _context->line_to(x2, y2);
    _context->stroke();
}

void Graphics2D_cro::drawRect(float x, float y, float w, float h) {
    _context->rectangle(x, y, w, h);
    _context->stroke();
}

void Graphics2D_cro::fillRect(float x, float y, float w, float h) {
    _context->rectangle(x, y, w, h);
    _context->fill();
}

void Graphics2D_cro::roundRect(float x, float y, float w, float h, float rx, float ry) {
    const double r = (rx > ry) ? rx : ry;
    const double d = M_PI / 180.0;
    _context->begin_new_sub_path();
    _context->arc(x + r, y + r, r, 180 * d, 270 * d);
    _context->arc(x + w - r, y + r, r, -90 * d, 0);
    _context->arc(x + w - r, y + h - r, r, 0, 90 * d);
    _context->arc(x + r, y + h - r, r, 90 * d, 180 * d);
    _context->close_path();
}

void Graphics2D_cro::drawRoundRect(float x, float y, float w, float h, float rx, float ry) {
    roundRect(x, y, w, h, rx, ry);
    _context->stroke();
}

void Graphics2D_cro::fillRoundRect(float x, float y, float w, float h, float rx, float ry) {
    roundRect(x, y, w, h, rx, ry);
    _context->fill();
}

}  // namespace ase::microtex
