cmake_minimum_required(VERSION 3.22)
project(RoneStucker VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)

# Fetch the WebView2 NuGet package for Windows. CI already downloads it and
# passes -DJUCE_WEBVIEW2_PACKAGE_LOCATION, so only fetch it ourselves when no
# location was supplied (local dev builds).
if(WIN32 AND NOT JUCE_WEBVIEW2_PACKAGE_LOCATION)
    FetchContent_Declare(
        webview2
        URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.1901.177
        DOWNLOAD_NO_EXTRACT FALSE
    )
    FetchContent_MakeAvailable(webview2)
    # JUCE's FindWebView2.cmake globs for *Microsoft.Web.WebView2* inside this dir,
    # so create a wrapper directory with the expected name using a directory junction.
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

juce_add_plugin(RoneStucker
    COMPANY_NAME           "RonePlugins"
    IS_SYNTH               FALSE
    NEEDS_MIDI_INPUT       FALSE
    NEEDS_MIDI_OUTPUT      FALSE
    IS_MIDI_EFFECT         FALSE
    PLUGIN_MANUFACTURER_CODE Rone
    PLUGIN_CODE            RnSk
    FORMATS                VST3 Standalone AU
    PRODUCT_NAME           "Rone Stucker"
    COPY_PLUGIN_AFTER_BUILD TRUE
    NEEDS_WEB_BROWSER      TRUE
    NEEDS_WEBVIEW2         TRUE
)

juce_generate_juce_header(RoneStucker)

target_sources(RoneStucker
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/RoneStuckerEngine.cpp
)

target_compile_definitions(RoneStucker
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(RoneStucker
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
