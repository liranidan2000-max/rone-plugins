# The RONE UI fonts, built into the plugin binary so the WebView page has its
# own type with no network (brand phase B, 26.10.2026; Stucker 1.3.4 did it
# first for itself, audit R076).
#
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../Shared/fonts/RoneFonts.cmake)
#   rone_add_fonts(<PluginTarget>)
#
# then serve them from the editor's resource provider with roneFontResource()
# (Shared/RoneFonts.h) and paste Shared/fonts/rone-fonts.css into the page.
#
# Latin woff2 subsets from @fontsource 5.3.0, SIL Open Font License 1.1
# (OFL-*.txt beside them): Sora, Manrope and Archivo as variable fonts (every
# weight in one file), IBM Plex Mono 400/500/600. About 140 KB in all.
#
# A hardware face that also needs Archivo Narrow / Roboto / Roboto Condensed names
# those files after the target and pastes rone-fonts-hardware.css as well:
#   rone_add_fonts(Rone2A roboto-latin-wght-normal.woff2 roboto-condensed-latin-wght-normal.woff2)

set(RONE_FONTS_DIR ${CMAKE_CURRENT_LIST_DIR})

function(rone_add_fonts target)
    set(_extra "")
    foreach(_f IN LISTS ARGN)
        list(APPEND _extra ${RONE_FONTS_DIR}/${_f})
    endforeach()
    juce_add_binary_data(${target}RoneFonts
        HEADER_NAME RoneFontsData.h
        NAMESPACE   RoneFontsData
        SOURCES
            ${RONE_FONTS_DIR}/sora-latin-wght-normal.woff2
            ${RONE_FONTS_DIR}/manrope-latin-wght-normal.woff2
            ${RONE_FONTS_DIR}/archivo-latin-wght-normal.woff2
            ${RONE_FONTS_DIR}/ibm-plex-mono-latin-400-normal.woff2
            ${RONE_FONTS_DIR}/ibm-plex-mono-latin-500-normal.woff2
            ${RONE_FONTS_DIR}/ibm-plex-mono-latin-600-normal.woff2
            ${_extra}
    )
    set_target_properties(${target}RoneFonts PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(${target} PRIVATE ${target}RoneFonts)
endfunction()
