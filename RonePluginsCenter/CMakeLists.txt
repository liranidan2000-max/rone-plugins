cmake_minimum_required(VERSION 3.22)
project(RonePluginsCenter VERSION 1.0.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- FetchContent setup ---
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE
)

# --- WebView2 SDK for Windows ---
# MUST be set up BEFORE FetchContent_MakeAvailable(JUCE) so JUCE can find WebView2.h
# CI passes -DJUCE_WEBVIEW2_PACKAGE_LOCATION=<path> which already contains
# Microsoft.Web.WebView2.x.x.x/build/native/include/WebView2.h — no extra work needed.
# For local builds without that flag, fetch from NuGet and create a wrapper.
if(WIN32 AND NOT DEFINED JUCE_WEBVIEW2_PACKAGE_LOCATION)
    FetchContent_Declare(
        webview2
        URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.1901.177
        DOWNLOAD_NAME webview2.zip
    )
    FetchContent_MakeAvailable(webview2)

    # JUCE's FindWebView2.cmake globs for Microsoft.Web.WebView2* inside this dir
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

# --- Now make JUCE available (after WebView2 is configured) ---
FetchContent_MakeAvailable(JUCE)

# --- GUI Application (not a plugin) ---
juce_add_gui_app(RonePluginsCenter
    PRODUCT_NAME  "RONE Plugins Center"
    COMPANY_NAME  "RONE Plugins"
    VERSION       "1.0.0"
    ICON_BIG      "${CMAKE_CURRENT_SOURCE_DIR}/Resources/AppIcon.png"
    ICON_SMALL    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/AppIcon.png"
)

target_sources(RonePluginsCenter PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/NetworkManager.cpp
    Source/VersionChecker.cpp
    Source/LicenseHandler.cpp
    Source/RoneTrayIcon.h
)

target_compile_definitions(RonePluginsCenter PRIVATE
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2=1
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:RonePluginsCenter,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:RonePluginsCenter,JUCE_VERSION>"
    JUCE_DISPLAY_SPLASH_SCREEN=0
    # URL for the remote version manifest — fetched on startup
    VERSIONS_JSON_URL="https://raw.githubusercontent.com/liranidan2000-max/rone-plugins/main/versions.json"
    # Windows Registry root for installed plugin versions
    RONE_REGISTRY_PATH="Software\\\\RONE\\\\Plugins"
    # Default standalone install folder
    RONE_INSTALL_DIR="RONE Plugins"
    # Default VST3 subfolder under Common Files\\VST3
    RONE_VST3_SUBDIR="RONE"
    # Windows Registry path for bundle license status
    RONE_LICENSE_REGISTRY_PATH="Software\\\\RONE\\\\License"
)

# --- Embed React UI + plugin logos as binary data ---
juce_add_binary_data(PluginsCenterUI SOURCES
    Resources/ui/dist/index.html
    Resources/ui/dist/bundle.js
    Resources/ui/dist/styles.css
    Resources/ReverseReverb_icon.png
    Resources/RoneStutter_icon.png
    Resources/RoneStemsFixer_icon.png
    Resources/RoneFlanger_icon.png
    Resources/AppIcon.png
)

target_link_libraries(RonePluginsCenter PRIVATE
    PluginsCenterUI
    juce::juce_core
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_events
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Generate JuceHeader.h — aggregates all linked JUCE modules into a single
# include. Without this call the header is never written to the build tree,
# causing C1083 "Cannot open include file: 'JuceHeader.h'" on every TU.
juce_generate_juce_header(RonePluginsCenter)

# Windows: link registry + shell APIs + WebView2 include path
if(WIN32)
    target_link_libraries(RonePluginsCenter PRIVATE advapi32 shell32 urlmon)

    # juce_add_gui_app doesn't support NEEDS_WEBVIEW2, so manually add include path
    if(DEFINED JUCE_WEBVIEW2_PACKAGE_LOCATION)
        file(GLOB _wv2_pkg "${JUCE_WEBVIEW2_PACKAGE_LOCATION}/Microsoft.Web.WebView2*")
        if(_wv2_pkg)
            target_include_directories(RonePluginsCenter PRIVATE "${_wv2_pkg}/build/native/include")
            message(STATUS "WebView2 include: ${_wv2_pkg}/build/native/include")
        endif()
    endif()
endif()
