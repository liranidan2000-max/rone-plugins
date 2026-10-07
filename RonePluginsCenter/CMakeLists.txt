cmake_minimum_required(VERSION 3.22)
project(RonePluginsCenter VERSION 2.0.6 LANGUAGES C CXX)

# Build nonce 2026-09-02-a. The v1.3.0 release push never produced a run: its
# commit body quoted the CI skip token while explaining an earlier commit, and
# GitHub scans the whole message, not just the subject. Never write that token
# in prose - say "without CI" instead.

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
    # The project version, never a second number: this one stayed "2.0.0" through
    # 2.0.4, so a Mac Center - which knows itself only by it - always saw an
    # update and reported every finished one as "did not finish" (2026-10-07).
    VERSION       ${PROJECT_VERSION}
    ICON_BIG      "${CMAKE_CURRENT_SOURCE_DIR}/Resources/AppIcon.png"
    ICON_SMALL    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/AppIcon.png"
    # ronecenter:// links from the website (Source/CenterLinks.h); JUCE hands
    # the URL to anotherInstanceStarted().
    PLIST_TO_MERGE [[<plist><dict>
      <key>CFBundleURLTypes</key>
      <array><dict>
        <key>CFBundleURLName</key><string>com.roneplugins.center</string>
        <key>CFBundleURLSchemes</key><array><string>ronecenter</string></array>
      </dict></array>
    </dict></plist>]]
)

target_sources(RonePluginsCenter PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/NetworkManager.cpp
    Source/VersionChecker.cpp
    Source/LicenseHandler.cpp
    Source/AccountClient.cpp
    Source/RoneTrayIcon.h
    Source/CustomTitleBar.h
    Source/ControlPort.h
    Source/InstallBatcher.h
    Source/DawDetector.h
    Source/CenterLinks.h
)

# Crash reports go to roneaudio.com/api/v1/crash, which holds the GitHub token
# on the server (Source/CrashReportUploader.h). Nothing secret is compiled in.

target_compile_definitions(RonePluginsCenter PRIVATE
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2=1
    # Static WebView2 loader: without this the app LoadLibrary()s
    # WebView2Loader.dll from its own folder, and every plugin installer that
    # ships the same DLL makes Windows Restart Manager force-close the running
    # Center mid-update ("Update All" kills the app).
    $<$<BOOL:${WIN32}>:JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1>
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:RonePluginsCenter,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:RonePluginsCenter,JUCE_VERSION>"
    JUCE_DISPLAY_SPLASH_SCREEN=0
    # URL for the remote version manifest — fetched on startup
    VERSIONS_JSON_URL="https://raw.githubusercontent.com/liranidan2000-max/rone-plugins/main/versions.json"
    # The same file straight from origin, for the one moment raw's CDN copy
    # cannot be trusted (a hash mismatch right after a release). Rate-limited
    # to 60/hour per IP unauthenticated, which is why it is not the default.
    VERSIONS_JSON_ORIGIN_URL="https://api.github.com/repos/liranidan2000-max/rone-plugins/contents/versions.json?ref=main"
    # Accounts API (sign-in, entitlement checks) — Cloudflare Pages Functions
    # served from the website itself, so it moves with the site.
    RONE_API_BASE="https://roneaudio.com/api/v1"
    # Windows Registry root for installed plugin versions
    RONE_REGISTRY_PATH="Software\\\\RONE\\\\Plugins"
    # Default standalone install folder
    RONE_INSTALL_DIR="RONE Plugins"
    # Default VST3 subfolder under Common Files\\VST3
    RONE_VST3_SUBDIR="RONE"
    # Windows Registry path for bundle license status
    RONE_LICENSE_REGISTRY_PATH="Software\\\\RONE\\\\License"
)

# --- Embed the React UI (page, script, styles, fonts), every plugin icon and
# every 3D unit as binary data. The page finds them by file name
# (MainComponent::getResource), so a new plugin's artwork is a file here and
# no code: Resources/<id>_icon.png and Resources/units/unit_<id>.webp.
# Only what the page uses: its html, script, styles and the woff2 fonts (the
# .woff copies @fontsource also emits are never fetched by WebView2/WebKit).
file(GLOB _ui_files   CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/Resources/ui/dist/*.html"
     "${CMAKE_CURRENT_SOURCE_DIR}/Resources/ui/dist/*.js"
     "${CMAKE_CURRENT_SOURCE_DIR}/Resources/ui/dist/*.css"
     "${CMAKE_CURRENT_SOURCE_DIR}/Resources/ui/dist/*.woff2")
file(GLOB _icon_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*_icon.png")
file(GLOB _unit_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/units/*.webp")
juce_add_binary_data(PluginsCenterUI SOURCES
    ${_ui_files}
    ${_icon_files}
    ${_unit_files}
    Resources/AppIcon.png
)

target_link_libraries(RonePluginsCenter PRIVATE
    PluginsCenterUI
    juce::juce_core
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_events
    juce::juce_cryptography
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# ControlPort.h (2.0.3): on Windows it opens its MIDI port through the system
# API, on the Mac through juce::MidiInput / MidiOutput - which live in
# juce_audio_devices. Without the module the Mac build of 2.0.3 failed
# (run #253: "no member named 'MidiOutput' in namespace 'juce'").
if (APPLE)
    target_link_libraries(RonePluginsCenter PRIVATE juce::juce_audio_basics juce::juce_audio_devices)
endif()

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

            # Static loader lib — required by JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                target_link_libraries(RonePluginsCenter PRIVATE "${_wv2_pkg}/build/native/x64/WebView2LoaderStatic.lib")
            else()
                target_link_libraries(RonePluginsCenter PRIVATE "${_wv2_pkg}/build/native/x86/WebView2LoaderStatic.lib")
            endif()
        endif()
    endif()
endif()
