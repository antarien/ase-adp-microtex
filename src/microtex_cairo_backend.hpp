#pragma once

/**
 * ASE MicroTeX Adapter — Cairo/Pango Graphics2D backend
 *
 * Concrete implementations of MicroTeX's abstract backend interfaces:
 *   • tex::Font         ⇒ Font_cro
 *   • tex::TextLayout   ⇒ TextLayout_cro
 *   • tex::Graphics2D   ⇒ Graphics2D_cro
 *
 * This is the one place in the codebase where `public tex::Font`,
 * `public tex::TextLayout`, and `public tex::Graphics2D` are permitted.
 * The ECS validator third-party whitelist authorizes these specifically
 * under the adapter/ase-microtex-adapter/ path.
 *
 * The implementation is a direct port of the upstream
 * NanoMichael/MicroTeX graphic_cairo.{h,cpp} to the gtkmm-4 stack
 * (cairomm-1.16 + pangomm-2.48). The upstream file cannot be used
 * directly because upstream links against gtkmm-3.0.
 *
 * @module      ase-microtex-adapter
 * @layer       adapter (third-party isolation)
 */

#include <cairomm/cairomm.h>
#include <pangomm.h>
#include <string>

#include "graphic/graphic.h"

namespace ase::microtex {

/**
 * Font handle backed by Cairo's FreeType font-face and a cached
 * family name from fontconfig.
 */
class Font_cro : public tex::Font {
public:
    Font_cro(std::string family, int style, float size);
    Font_cro(const std::string& file, float size);

    float getSize() const override { return _size; }
    tex::sptr<tex::Font> deriveFont(int style) const override;
    bool operator==(const tex::Font& f) const override;
    bool operator!=(const tex::Font& f) const override;

    const std::string& getFamily() const { return _family; }
    int getStyle() const { return _style; }
    Cairo::RefPtr<Cairo::FtFontFace> getCairoFontFace() const { return _fface; }

private:
    void loadFont(const std::string& file);

    std::string _family;
    int _style;
    float _size;
    Cairo::RefPtr<Cairo::FtFontFace> _fface;
};

/**
 * Text layout for runs that MicroTeX cannot shape itself (labels,
 * identifiers, external Unicode). Backed by Pango.
 */
class TextLayout_cro : public tex::TextLayout {
public:
    TextLayout_cro(const std::wstring& src, const tex::sptr<Font_cro>& font);

    void getBounds(tex::Rect& r) override;
    void draw(tex::Graphics2D& g2, float x, float y) override;

private:
    Glib::RefPtr<Pango::Layout> _layout;
    float _ascent;
};

/**
 * Graphics2D backend. Delegates every primitive to the supplied
 * cairomm Context; translates MicroTeX Stroke/Cap/Join/Font enums
 * into their cairomm-1.16 counterparts.
 */
class Graphics2D_cro : public tex::Graphics2D {
public:
    explicit Graphics2D_cro(const Cairo::RefPtr<Cairo::Context>& context);

    const Cairo::RefPtr<Cairo::Context>& getCairoContext() const { return _context; }

    void setColor(tex::color c) override;
    tex::color getColor() const override;
    void setStroke(const tex::Stroke& s) override;
    const tex::Stroke& getStroke() const override;
    void setStrokeWidth(float w) override;

    const tex::Font* getFont() const override;
    void setFont(const tex::Font* font) override;

    void translate(float dx, float dy) override;
    void scale(float sx, float sy) override;
    void rotate(float angle) override;
    void rotate(float angle, float px, float py) override;
    void reset() override;
    float sx() const override { return _sx; }
    float sy() const override { return _sy; }

    void drawChar(wchar_t c, float x, float y) override;
    void drawText(const std::wstring& t, float x, float y) override;
    void drawLine(float x1, float y1, float x2, float y2) override;
    void drawRect(float x, float y, float w, float h) override;
    void fillRect(float x, float y, float w, float h) override;
    void drawRoundRect(float x, float y, float w, float h, float rx, float ry) override;
    void fillRoundRect(float x, float y, float w, float h, float rx, float ry) override;

private:
    void roundRect(float x, float y, float w, float h, float rx, float ry);

    Cairo::RefPtr<Cairo::Context> _context;
    tex::color _color;
    tex::Stroke _stroke;
    const Font_cro* _font;
    float _sx, _sy;
};

}  // namespace ase::microtex
