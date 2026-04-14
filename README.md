# ase-adp-microtex

[![Layer](https://img.shields.io/badge/Layer-Adapter-orange.svg)]()
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()
[![Kind](https://img.shields.io/badge/Kind-3rd%20Party%20Isolation-red.svg)]()

> MicroTeX (LaTeX math) Cairo/Pango backend adapter — isolates `tex::*` inheritance from client code

Part of [ASE - Antares Simulation Engine](../../..)

## Why this module exists

ASE clients render LaTeX math in technical documentation (formulas, equations, matrices). [MicroTeX](https://github.com/NanoMichael/MicroTeX) is the rendering engine of choice because it covers the full LaTeX math subset including matrices, alignments, and stretchy delimiters. However, MicroTeX's pluggable backend architecture is inheritance-based: every platform backend must subclass `tex::Graphics2D`, `tex::Font`, and `tex::TextLayout` (pure virtual interfaces).

ASE client code is inheritance-hostile by policy. The ECS validator enforces this: classes may not inherit from arbitrary third-party types. An adapter module is the architectural answer: this subgit is the one place where `public tex::Font` / `public tex::Graphics2D` / `public tex::TextLayout` is permitted, and clients import only the clean ASE-native surface.

## Public API

Clients include only `ase/adp/microtex/*.hpp` and never touch `tex::*` types.

```cpp
#include <ase/adp/microtex/init.hpp>
#include <ase/adp/microtex/render.hpp>

// Once at process startup:
ase::microtex::init();

// Per draw (inside a Cairo::DrawingArea draw handler or similar):
auto result = ase::microtex::render_math(
    cr,                      // Cairo::RefPtr<Cairo::Context>
    "\\frac{a^2 + b^2}{c}",  // LaTeX source
    x_position,
    y_position,
    font_size_px,
    /* is_display = */ true
);
// result.width, result.height, result.depth
```

## Architecture

- **Upstream:** NanoMichael/MicroTeX core sources (atom/, box/, core/, fonts/, utils/, res/, latex.cpp, render.cpp). The upstream `CMakeLists.txt` is NOT used — it hardcodes `gtkmm-3.0` / `cairomm-1.0` / `gtksourceviewmm-3.0` which conflict with the viewer's `gtkmm-4.0` stack. We compile the portable core sources manually and ship our own `Graphics2D` backend.
- **Backend:** `cairomm-1.16` + `pangomm-2.48` (gtkmm-4 compatible), wired by `src/microtex_cairo_backend.cpp`.
- **Fonts / symbols:** MicroTeX loads fonts, glyph metrics, and formula mappings from a `res/` tree at runtime via `tex::LaTeX::init(res_path)`. The adapter's CMake copies the upstream `res/` tree into `${CMAKE_CURRENT_BINARY_DIR}/share/microtex/res` and hardcodes the path as a compile-time define.
- **Public surface:** `ase::microtex::MathResult`, `ase::microtex::render_math()`, `ase::microtex::init()`. No `tex::*` symbol is exported from the public headers.

## Layer

`adapter/` is a new top-level directory — a third-party isolation layer that sits orthogonal to the `L0..L5` ECS stack. Consumed by L5 clients via `ase::microtex`. Depends on `cairomm-1.16` and `pangomm-2.48` as system dependencies, and on the fetched MicroTeX sources as a FetchContent dep.

## Build

Built as part of the ASE root build when `ASE_BUILD_CLIENTS=ON`. Standalone:

```bash
cd adapter/ase-adp-microtex
cmake -B build -G Ninja
ninja -C build
```

## Adding future adapters

New 3rd-party-inheritance-heavy libraries (Qt, Skia, Wayland-native, ...) get their own subgits under `adapter/`:

- `adapter/ase-qt-adapter/` — if Qt ever becomes a secondary rendering path
- `adapter/ase-skia-adapter/` — if Skia is chosen for cross-platform 2D
- `adapter/ase-<lib>-adapter/` — pattern

Each adapter isolates its third-party inheritance entirely, exposes only ASE-native types, and keeps client subgits inheritance-clean.
