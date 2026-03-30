cmake_minimum_required(VERSION 3.22)
project(RonePluginsCenter VERSION 1.0.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- JUCE via FetchContent ---
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# --- WebView2 SDK for Windows ---
if(WIN32)
    FetchContent_Declare(
        WebView2
        URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2903.40
        URL_HASH SHA256=0
        DOWNLOAD_NO_EXTRACT FALSE
    )
    FetchContent_MakeAvailable(WebView2)

    # Point JUCE to the WebView2 package
    set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${webview2_SOURCE_DIR}" CACHE STRING "" FORCE)
endif()

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

# Windows: link registry + shell APIs
if(WIN32)
    target_link_libraries(RonePluginsCenter PRIVATE advapi32 shell32 urlmon)
endif()
