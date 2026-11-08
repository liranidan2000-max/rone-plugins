cmake_minimum_required(VERSION 3.22)
project(RoneAfterspace VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)

# Fetch WebView2 NuGet package for Windows
if(WIN32)
    FetchContent_Declare(
        webview2
        URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.1901.177
        DOWNLOAD_NO_EXTRACT FALSE
    )
    FetchContent_MakeAvailable(webview2)
    set(_wv2_wrapper "${CMAKE_BINARY_DIR}/_webview2_wrapper")
    set(_wv2_link "${_wv2_wrapper}/Microsoft.Web.WebView2")
    if(NOT EXISTS "${_wv2_link}/build/native/include/WebView2.h")
        file(MAKE_DIRECTORY "${_wv2_wrapper}")
        cmake_path(NATIVE_PATH webview2_SOURCE_DIR _wv2_native_src)
        cmake_path(NATIVE_PATH _wv2_link _wv2_native_link)
        execute_process(COMMAND cmd /c mklink /J "${_wv2_native_link}" "${_wv2_native_src}")
    endif()
    set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${_wv2_wrapper}" CACHE STRING "" FORCE)
endif()

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(RoneAfterspace
    COMPANY_NAME           "RonePlugins"
    IS_SYNTH               FALSE
    NEEDS_MIDI_INPUT       FALSE
    NEEDS_MIDI_OUTPUT      FALSE
    IS_MIDI_EFFECT         FALSE
    PLUGIN_MANUFACTURER_CODE Rone
    PLUGIN_CODE            RnAS
    FORMATS                VST3 Standalone AU
    # No moduleinfo.json step: JUCE's juce_vst3_helper loads the freshly built plugin at
    # build time and crashes at random on the CI runners (runs #167/#168, macOS). The file
    # is optional - hosts scan the binary instead, as they do for most plugins.
    VST3_AUTO_MANIFEST                FALSE
    PRODUCT_NAME           "RONE AFTERSPACE"
    # macOS: without a usage description the system kills an app the moment it opens the microphone
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "RONE AFTERSPACE needs your audio input to process live audio."
    COPY_PLUGIN_AFTER_BUILD FALSE
    NEEDS_WEB_BROWSER      TRUE
    NEEDS_WEBVIEW2         TRUE
    ICON_BIG               "${CMAKE_CURRENT_SOURCE_DIR}/Source/icon_1024.png"
    ICON_SMALL             "${CMAKE_CURRENT_SOURCE_DIR}/Source/icon_256.png"
)

# ---------------------------------------------------------------------------
# RONE Control Rack (DEV, branch rack-dev): RONE Control's hands beside the plugin.
# The rack library AND RONE Control's hands layer (mapper, link hub, link view) are one folder in the
# monorepo, Shared/RoneControlRack (since 26.10.2026) - the same code RONE Control itself compiles.
# The hand model is NOT embedded: it lives once, in RONE Control, and the rack reads the files RONE
# Control installed (without them the rack shows "install RONE Control from the Center").
# ---------------------------------------------------------------------------
set(RACK_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/../Shared/RoneControlRack")
set(RACK_CORE_DIR "${RACK_DIR}")
if(NOT EXISTS "${RACK_DIR}/RackEngine.cpp")
    message(FATAL_ERROR "The rack needs the monorepo's Shared/RoneControlRack next to the plugin (${RACK_DIR})")
endif()
juce_add_binary_data(RoneAfterspaceRackData
    SOURCES
        "${RACK_DIR}/ui/rack.html"
        "${RACK_DIR}/ui/rack.css"
        "${RACK_DIR}/ui/rack.js"
        "${RACK_DIR}/ui/vendor/qrcode.min.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/rack.json")
target_sources(RoneAfterspace
    PRIVATE
        "${RACK_DIR}/RackEngine.cpp"
        "${RACK_DIR}/RackView.cpp"
        "${RACK_DIR}/RackEditorHelper.cpp"
        "${RACK_DIR}/RackTransport.cpp"
        "${RACK_CORE_DIR}/ControlCore.cpp"
        "${RACK_CORE_DIR}/HandLink.cpp"
        "${RACK_CORE_DIR}/LinkHub.cpp"
        "${RACK_CORE_DIR}/LinkView.cpp")
target_include_directories(RoneAfterspace PRIVATE "${RACK_DIR}" "${RACK_CORE_DIR}")
target_link_libraries(RoneAfterspace PRIVATE RoneAfterspaceRackData)

juce_generate_juce_header(RoneAfterspace)

target_sources(RoneAfterspace
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        ../Shared/RoneReverbEngine.cpp   # the FDN reverb (Source/dsp/ReverbEngine.h names it ReverbEngine)
        Source/dsp/EchoEngine.cpp
        Source/dsp/DuckProcessor.cpp
)

target_compile_definitions(RoneAfterspace
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# The RONE UI fonts built into the binary (brand phase B, Shared/fonts)
include(${CMAKE_CURRENT_SOURCE_DIR}/../Shared/fonts/RoneFonts.cmake)
rone_add_fonts(RoneAfterspace)

target_link_libraries(RoneAfterspace
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# =============================================================================
# DSP smoke tests (spec §26 /tests) — build explicitly:
#   cmake --build build --target AfterspaceTests
# =============================================================================
juce_add_console_app(AfterspaceTests PRODUCT_NAME "AfterspaceTests")
set_target_properties(AfterspaceTests PROPERTIES EXCLUDE_FROM_ALL TRUE)
juce_generate_juce_header(AfterspaceTests)

target_sources(AfterspaceTests
    PRIVATE
        Source/tests/DSPTests.cpp
        ../Shared/RoneReverbEngine.cpp
        Source/dsp/EchoEngine.cpp
        Source/dsp/DuckProcessor.cpp
)

target_compile_definitions(AfterspaceTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(AfterspaceTests
    PRIVATE
        juce::juce_audio_basics
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
