# rone-preview-render: the offline host behind the Plugins Center's dry/wet
# previews (see main.cpp and render_previews.py). A dev tool: never shipped.
#
#   cmake -B build -DJUCE_DIR=<path to JUCE>   (e.g. a Center build's _deps/juce-src)
#   cmake --build build --config Release
cmake_minimum_required(VERSION 3.22)
project(RonePreviewRender VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

if(NOT JUCE_DIR)
    set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../RonePluginsCenter/build-vs2022/_deps/juce-src")
endif()
add_subdirectory("${JUCE_DIR}" juce)

juce_add_console_app(RonePreviewRender PRODUCT_NAME "rone-preview-render")
target_sources(RonePreviewRender PRIVATE main.cpp)
target_compile_definitions(RonePreviewRender PRIVATE
    JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_AU=0 JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
target_link_libraries(RonePreviewRender PRIVATE
    juce::juce_audio_processors juce::juce_audio_formats juce::juce_audio_utils
    juce::juce_recommended_config_flags)
