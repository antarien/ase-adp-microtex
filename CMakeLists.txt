cmake_minimum_required(VERSION 3.22)

#[[
ASE MICROTEX ADAPTER BUILD CONFIGURATION

@file        CMakeLists.txt
@brief       Build configuration for ase-adp-microtex
@description 3rd-party isolation module for MicroTeX LaTeX math rendering.
             Wraps tex::Graphics2D / tex::Font / tex::TextLayout inheritance
             behind a clean ase::adp::microtex public API built on cairomm-1.16
             and pangomm-2.48.

@module      ase-adp-microtex
@layer       adapter (3rd-party isolation, consumed by L5 clients)
@created     2026-04-13
@modified    2026-04-13
@version     00.00.01.00001 [stub]
]]

# Parse local VERSION (SSOT) if not building from root
if(NOT DEFINED ASE_ADP_MICROTEX_VERSION)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
        message(FATAL_ERROR "VERSION file not found: ${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    endif()
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _VERSION_LINES)
    foreach(_LINE ${_VERSION_LINES})
        if(_LINE MATCHES "^ASE_ADP_MICROTEX=(.*)$")
            set(ASE_ADP_MICROTEX_VERSION "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(NOT DEFINED ASE_ADP_MICROTEX_VERSION)
        message(FATAL_ERROR "ASE_ADP_MICROTEX version not found in VERSION file")
    endif()
endif()

project(ase-adp-microtex VERSION ${ASE_ADP_MICROTEX_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

message(STATUS "Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")

# ── System dependencies ─────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIROMM REQUIRED IMPORTED_TARGET cairomm-1.16)
pkg_check_modules(PANGOMM REQUIRED IMPORTED_TARGET pangomm-2.48)
pkg_check_modules(TINYXML2 REQUIRED IMPORTED_TARGET tinyxml2)
pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)

# ── Fetch MicroTeX sources ──────────────────────────────────────────
# When built as part of the ASE root build, the `microtex` FetchContent
# target is already declared by the root CMakeLists. When built standalone,
# we declare it here as a fallback (same GIT_TAG, shared cache).
#
# The upstream CMakeLists is bypassed via SOURCE_SUBDIR pointing to a
# non-existent path — we only want the sources, not upstream's broken
# gtkmm-3.0 / cairomm-1.0 configuration.
if(NOT microtex_SOURCE_DIR)
    include(FetchContent)
    FetchContent_Declare(microtex
        GIT_REPOSITORY https://github.com/NanoMichael/MicroTeX.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR _skip_upstream_cmake_
    )
    FetchContent_MakeAvailable(microtex)
endif()

# ── MicroTeX core static library ────────────────────────────────────
# Compile only the portable core (parser, boxes, atoms, fonts). No
# platform backend — we provide our own via microtex_cairo_backend.cpp.
if(NOT TARGET microtex_core)
    set(MTX_SRC ${microtex_SOURCE_DIR}/src)
    add_library(microtex_core STATIC
        #[[ atom ]]
        ${MTX_SRC}/atom/atom_basic.cpp
        ${MTX_SRC}/atom/atom_char.cpp
        ${MTX_SRC}/atom/atom_impl.cpp
        ${MTX_SRC}/atom/atom_matrix.cpp
        ${MTX_SRC}/atom/atom_row.cpp
        ${MTX_SRC}/atom/atom_space.cpp
        ${MTX_SRC}/atom/colors_def.cpp
        ${MTX_SRC}/atom/unit_conversion.cpp
        #[[ box ]]
        ${MTX_SRC}/box/box.cpp
        ${MTX_SRC}/box/box_factory.cpp
        ${MTX_SRC}/box/box_group.cpp
        ${MTX_SRC}/box/box_single.cpp
        #[[ core ]]
        ${MTX_SRC}/core/core.cpp
        ${MTX_SRC}/core/formula.cpp
        ${MTX_SRC}/core/formula_def.cpp
        ${MTX_SRC}/core/glue.cpp
        ${MTX_SRC}/core/localized_num.cpp
        ${MTX_SRC}/core/macro.cpp
        ${MTX_SRC}/core/macro_def.cpp
        ${MTX_SRC}/core/macro_impl.cpp
        ${MTX_SRC}/core/parser.cpp
        #[[ fonts ]]
        ${MTX_SRC}/fonts/alphabet.cpp
        ${MTX_SRC}/fonts/font_basic.cpp
        ${MTX_SRC}/fonts/font_info.cpp
        ${MTX_SRC}/fonts/fonts.cpp
        #[[ utils ]]
        ${MTX_SRC}/utils/string_utils.cpp
        ${MTX_SRC}/utils/utf.cpp
        ${MTX_SRC}/utils/utils.cpp
        #[[ res (symbol / formula / font mappings generated at build of upstream) ]]
        ${MTX_SRC}/res/builtin/formula_mappings.res.cpp
        ${MTX_SRC}/res/builtin/symbol_mapping.res.cpp
        ${MTX_SRC}/res/builtin/tex_param.res.cpp
        ${MTX_SRC}/res/builtin/tex_symbols.res.cpp
        ${MTX_SRC}/res/font/bi10.def.cpp
        ${MTX_SRC}/res/font/bx10.def.cpp
        ${MTX_SRC}/res/font/cmbsy10.def.cpp
        ${MTX_SRC}/res/font/cmbx10.def.cpp
        ${MTX_SRC}/res/font/cmbxti10.def.cpp
        ${MTX_SRC}/res/font/cmex10.def.cpp
        ${MTX_SRC}/res/font/cmmi10.def.cpp
        ${MTX_SRC}/res/font/cmmi10_unchanged.def.cpp
        ${MTX_SRC}/res/font/cmmib10.def.cpp
        ${MTX_SRC}/res/font/cmmib10_unchanged.def.cpp
        ${MTX_SRC}/res/font/cmr10.def.cpp
        ${MTX_SRC}/res/font/cmss10.def.cpp
        ${MTX_SRC}/res/font/cmssbx10.def.cpp
        ${MTX_SRC}/res/font/cmssi10.def.cpp
        ${MTX_SRC}/res/font/cmsy10.def.cpp
        ${MTX_SRC}/res/font/cmti10.def.cpp
        ${MTX_SRC}/res/font/cmti10_unchanged.def.cpp
        ${MTX_SRC}/res/font/cmtt10.def.cpp
        ${MTX_SRC}/res/font/dsrom10.def.cpp
        ${MTX_SRC}/res/font/eufb10.def.cpp
        ${MTX_SRC}/res/font/eufm10.def.cpp
        ${MTX_SRC}/res/font/i10.def.cpp
        ${MTX_SRC}/res/font/moustache.def.cpp
        ${MTX_SRC}/res/font/msam10.def.cpp
        ${MTX_SRC}/res/font/msbm10.def.cpp
        ${MTX_SRC}/res/font/r10.def.cpp
        ${MTX_SRC}/res/font/r10_unchanged.def.cpp
        ${MTX_SRC}/res/font/rsfs10.def.cpp
        ${MTX_SRC}/res/font/sb10.def.cpp
        ${MTX_SRC}/res/font/sbi10.def.cpp
        ${MTX_SRC}/res/font/si10.def.cpp
        ${MTX_SRC}/res/font/special.def.cpp
        ${MTX_SRC}/res/font/ss10.def.cpp
        ${MTX_SRC}/res/font/stmary10.def.cpp
        ${MTX_SRC}/res/font/tt10.def.cpp
        ${MTX_SRC}/res/parser/font_parser.cpp
        ${MTX_SRC}/res/parser/formula_parser.cpp
        ${MTX_SRC}/res/reg/builtin_font_reg.cpp
        ${MTX_SRC}/res/reg/builtin_syms_reg.cpp
        ${MTX_SRC}/res/sym/amsfonts.def.cpp
        ${MTX_SRC}/res/sym/amssymb.def.cpp
        ${MTX_SRC}/res/sym/base.def.cpp
        ${MTX_SRC}/res/sym/stmaryrd.def.cpp
        ${MTX_SRC}/res/sym/symspecial.def.cpp
        #[[ public API entry points ]]
        ${MTX_SRC}/latex.cpp
        ${MTX_SRC}/render.cpp
    )
    target_include_directories(microtex_core PUBLIC ${MTX_SRC})
    target_compile_features(microtex_core PUBLIC cxx_std_20)
    target_compile_definitions(microtex_core PUBLIC HAVE_LOG)
    target_link_libraries(microtex_core PRIVATE PkgConfig::TINYXML2)
    set_target_properties(microtex_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # MicroTeX loads its font/symbol resources from a res/ tree at runtime.
    # Copy the upstream res/ into the build tree and inject the absolute path
    # as a compile-time define consumed by microtex_init.cpp.
    file(COPY ${microtex_SOURCE_DIR}/res DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/share/microtex)
endif()

# ── Adapter library ─────────────────────────────────────────────────
add_library(ase-adp-microtex STATIC
    src/microtex_cairo_backend.cpp
    src/microtex_render.cpp
    src/microtex_init.cpp

    include/ase/adp/microtex/microtex.hpp
    include/ase/adp/microtex/init.hpp
    include/ase/adp/microtex/render.hpp
)
add_library(ase::adp::microtex ALIAS ase-adp-microtex)

target_include_directories(ase-adp-microtex
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(ase-adp-microtex PUBLIC cxx_std_20)

target_link_libraries(ase-adp-microtex
    PUBLIC
        PkgConfig::CAIROMM
    PRIVATE
        microtex_core
        PkgConfig::PANGOMM
        PkgConfig::FONTCONFIG
)

target_compile_definitions(ase-adp-microtex PRIVATE
    ASE_MICROTEX_RES_PATH="${CMAKE_CURRENT_BINARY_DIR}/share/microtex/res"
)

include(GNUInstallDirs)
install(TARGETS ase-adp-microtex DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
